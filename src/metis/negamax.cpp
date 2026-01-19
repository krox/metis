#include "metis/negamax.h"

#include "metis/evaluator.h"
#include "metis/move_generator.h"
#include "util/json.h"
#include <memory>

namespace metis {

NegamaxEngine::NegamaxEngine(std::shared_ptr<Evaluator> const &evaluator,
                             Options const &opts)
    : options_{opts}, eval_(evaluator)
{}

std::optional<int> NegamaxEngine::search(Board const &board, int depth,
                                         int alpha, int beta)
{
	// note: 'depth' can be below zero inside quiescence search.
	// note: illegal positions dont count into "node_count": Move-generation
	// creating some illegal moves is an implementation detail. Future:
	// cache-hits do count.
	if (!board.legal())
		return -32000;

	node_count_++;

	auto should_stop = [&]() -> bool {
		if (node_count_ >= node_limit_)
			return true;
		if (node_count_ % 100 != 0) // rate-limit non-trivial checks
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
	if (board.draw())
		return 0;

	int best_score = -32000;

	if (depth <= 0)
	{
		node_count_++;
		best_score = eval_->evaluate(board);
		if (board.color_to_move != Color::White)
			best_score = -best_score;
		best_score = std::clamp(best_score, -10000, 10000);
		if (best_score >= beta || !options_.qsearch)
			return best_score;
	}

	MoveList moves;
	generate_pseudolegal_moves(board, moves, depth <= 0);
	for (auto move : moves)
	{
		auto score =
		    search(board, move, depth, std::max(alpha, best_score + 1), beta);

		if (!score)
			return {};

		if (*score > beta)
			return score;

		if (*score > best_score)
			best_score = *score;
	}
	return best_score;
}

std::optional<int> NegamaxEngine::search(Board board, Move move, int depth,
                                         int alpha, int beta)
{
	board.make_move(move);
	auto score = search(board, depth - 1, -beta, -alpha);
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
                          std::stop_token stoken, int time_limit)
{
	node_count_ = 0;
	start_time_ = Clock::now();
	node_limit_ = INT_MAX; // TODO
	time_limit_ = time_limit;
	stoken_ = stoken;
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
	}

	progress({.best_move = best_move});
}

} // namespace metis