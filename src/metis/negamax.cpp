#include "metis/negamax.h"

#include "metis/evaluator.h"
#include "metis/move_generator.h"

namespace metis {

int NegamaxEngine::search(Board const &board, int depth, int alpha, int beta,
                          std::stop_token const &stoken)
{
	if (board.checkmate())
		return -31000;
	if (board.draw())
		return 0;
	if (depth <= 0)
	{
		auto eval = eval_->evaluate(board);
		if (board.color_to_move != Color::White)
			eval = -eval;
		eval = std::clamp(eval, -10000, 10000);
		return eval;
	}

	int best_score = -32000;
	MoveList moves;
	generate_pseudolegal_moves(board, moves);
	for (auto move : moves)
	{
		auto score = search(board, move, depth - 1,
		                    std::max(alpha, best_score + 1), beta, stoken);

		if (score > beta)
			return score;

		if (score > best_score)
			best_score = score;
	}
	return best_score;
}

int NegamaxEngine::search(Board board, Move move, int depth, int alpha,
                          int beta, std::stop_token const &stoken)
{
	board.make_move(move);
	if (!board.legal())
		return -32000;
	auto score = -search(board, depth, -beta, -alpha, stoken);

	// mate-scores are adjusted to favor shorter mates
	if (score > 30000)
		score -= 1;
	if (score < -30000)
		score += 1;
	return score;
}

void NegamaxEngine::think(Board const &board, ProgressCallback progress,
                          std::stop_token stoken)
{
	int slack = 5;

	MoveList moves;
	generate_pseudolegal_moves(board, moves);
	erase_if(moves, [&](Move const &move) {
		auto new_board = board;
		new_board.make_move(move);
		return !new_board.legal();
	});

	assert(!moves.empty());
	if (moves.size() == 1)
	{
		progress({.best_move = moves[0]});
		return;
	}

	int best_score = -32000;
	Move best_move = moves[0];
	int second_best_score = -32000;
	Move second_best_move = moves[1];

	for (int depth = 0; depth <= depth_limit_; ++depth)
	{
		// search the best move from previous depth
		best_score = search(board, best_move, depth - 1, -32000, 32000, stoken);
		second_best_score = -32000;

		for (auto move : moves)
		{
			if (move == best_move)
				continue;

			auto score = search(board, move, depth - 1, second_best_score + 1,
			                    32000, stoken);
			if (score > best_score)
			{
				second_best_score = best_score;
				second_best_move = best_move;
				best_score = score;
				best_move = move;
			}
			else if (score > second_best_score)
			{
				second_best_score = score;
				second_best_move = move;
			}
		}

		auto chosen_move = best_move;
		if (-20000 < best_score && best_score < 20000)
			if (second_best_score > best_score - slack)
				if (rng.bernoulli())
					chosen_move = second_best_move;
		progress({.best_move = chosen_move});

		if (stoken.stop_requested())
			return;
	}
}

} // namespace metis