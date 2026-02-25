#include "metis/negamax.h"

#include "metis/evaluator.h"
#include "metis/move_generator.h"
#include "util/json.h"
#include <memory>

namespace metis {

NegamaxEngine::NegamaxEngine(std::shared_ptr<Evaluator> const &evaluator,
                             Options const &opts)
    : options_{opts}, eval_(evaluator), cache_(/*MiB=*/64)
{}

bool NegamaxEngine::is_threefold_repetition(Board const &board) const
{
	auto key = board.zobrist();
	int repetitions = 0;
	for (auto const &pos : state_.history)
		if (pos == key)
			++repetitions;
	return repetitions >= 3;
}

std::optional<int> NegamaxEngine::search(Board const &board, int depth,
                                         int alpha, int beta)
{
	assert(depth >= 0);
	// note: illegal positions dont count into "node_count" because the fact
	// that our move-generator produces some illegal moves is an implementation
	// detail. A cache-hit counts as a single nodes though.
	if (!board.legal())
		return -32000;

	node_count_++;

	auto should_stop = [&]() -> bool {
		if (node_count_ >= node_limit_)
			return true;
		if (node_count_ % 8 != 0) // rate-limit non-trivial checks
			return false;
		if (stoken_.stop_requested())
			return true;
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		                   Clock::now() - start_time_)
		                   .count();
		if (elapsed >= time_limit_)
			return true;
		return false;
	};
	if (should_stop())
		return {};

	if (board.checkmate())
		return -31000;
	// note: important to check repetition-draw before cache
	if (is_threefold_repetition(board))
		return 0;
	if (board.draw())
		return 0;

	Move pv_move = Move::null();
	if (auto cached = cache_.probe(board.zobrist()); cached)
	{
		pv_move = cached->pv_move; // might be null
		if (cached->depth >= depth)
		{
			auto score = cached->score;
			if (cached->flags == Cache::Bound::Exact)
				return score;
			if (cached->flags == Cache::Bound::Lower && score >= beta)
				return score;
			if (cached->flags == Cache::Bound::Upper && score <= alpha)
				return score;
		}
	}

	int best_score = -32000;
	Move best_move = Move::null();

	if (depth <= 0)
	{
		best_score = eval_->evaluate(board);
		if (board.color_to_move != Color::White)
			best_score = -best_score;
		best_score = std::clamp(best_score, -10000, 10000);
		if (best_score >= beta || !options_.qsearch)
			return best_score;
	}

	// note: search PV move before move generation. hoping for a beta-cutoff.
	if (pv_move != Move::null())
	{
		auto score = search(board, pv_move, depth, alpha, beta);
		if (!score)
			return {};
		if (*score > beta)
		{
			cache_.store(board.zobrist(), depth, *score, Cache::Bound::Lower,
			             pv_move);
			return score;
		}

		best_score = *score;
		best_move = pv_move;
	}

	MoveList moves;
	generate_pseudolegal_moves(board, moves, depth <= 0);

	for (auto move : moves)
	{
		// pv move was already searched before
		if (move == pv_move)
			continue;

		auto score =
		    search(board, move, depth, std::max(alpha, best_score + 1), beta);

		if (!score)
			return {};

		if (*score > beta)
		{
			cache_.store(board.zobrist(), depth, *score, Cache::Bound::Lower,
			             move);
			return score;
		}

		if (*score > best_score)
		{
			best_score = *score;
			best_move = move;
		}
	}

	auto bound = Cache::Bound::Exact;
	if (best_score <= alpha)
		bound = Cache::Bound::Upper;
	else if (best_score >= beta)
		bound = Cache::Bound::Lower;
	if (depth > 0)
		cache_.store(board.zobrist(), depth, best_score, bound, best_move);
	return best_score;
}

std::optional<int> NegamaxEngine::search(Board board, Move move, int depth,
                                         int alpha, int beta)
{
	board.make_move(move);
	if (!board.legal())
		return -32000;
	state_.history.push_back(board.zobrist());
	auto score = search(board, std::max(0, depth - 1), -beta, -alpha);
	state_.history.pop_back();
	if (!score)
		return {};
	*score = -*score;

	// mate-scores are adjusted to favor shorter mates
	if (*score > 30000)
		*score -= 1;
	if (*score < -30000)
		*score += 1;
	return *score;
}

void NegamaxEngine::think(Board const &board, ProgressCallback progress,
                          std::stop_token stoken, int time_limit,
                          std::span<uint64_t const> history)
{
	node_count_ = 0;
	start_time_ = Clock::now();
	node_limit_ = INT_MAX; // TODO
	time_limit_ = time_limit;
	stoken_ = stoken;
	state_.history.clear();
	state_.board = board;
	state_.history.assign(history.begin(), history.end());
	if (state_.history.empty() || state_.history.back() != board.zobrist())
		state_.history.push_back(board.zobrist());
	int depth_limit = 80; // safeguard

	// generate all legal moves
	MoveList moves;
	generate_pseudolegal_moves(board, moves);
	erase_if(moves, [&](Move const &move) {
		auto new_board = board;
		new_board.make_move(move);
		return !new_board.legal();
	});

	// only one legal move: no need to search
	assert(!moves.empty());
	if (moves.size() == 1)
	{
		progress({.best_move = moves[0]});
		return;
	}

	int best_score = -32000;
	Move best_move = moves[0];

	for (int depth = 1; depth <= depth_limit; ++depth)
	{
		// full search the best move from previous depth
		if (auto score = search(board, best_move, depth, -32000, 32000))
			best_score = *score;
		else
			break;

		// search alternative moves with alpha-pruning
		for (auto move : moves)
		{
			if (move == best_move)
				continue;

			auto score = search(board, move, depth, best_score + 1, 32000);
			if (!score)
				break;

			if (*score > best_score)
			{
				best_score = *score;
				best_move = move;
			}
		}

		AnalysisResult result;
		result.best_move = best_move;
		result.score = best_score;
		result.depth = depth;
		result.nodes = node_count_;
		progress(result);

		// found a mate, no need to search deeper.
		// TODO: this does not guarantee that we found the shortest mate.
		if (best_score >= 30000 || best_score <= -30000)
			break;
	}
}

} // namespace metis