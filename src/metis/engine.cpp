#include "metis/engine.h"

#include "metis/negamax.h"

namespace metis {

void RandomEngine::think(Board const &board, ProgressCallback progress,
                         std::stop_token stoken)
{
	(void)stoken;

	MoveList moves;
	generate_pseudolegal_moves(board, moves);

	erase_if(moves, [&](Move move) {
		auto new_board = board;
		new_board.make_move(move);
		return !new_board.legal();
	});

	assert(!moves.empty());
	progress({.best_move = moves[rng() % moves.size()]});
}

void MateInOneEngine::think(Board const &board, ProgressCallback progress,
                            std::stop_token stoken)
{
	(void)stoken;

	MoveList moves;
	generate_pseudolegal_moves(board, moves);

	MoveList candidates;
	for (auto move : moves)
	{
		auto new_board = board;
		new_board.make_move(move);
		if (!new_board.legal())
			continue;

		if (new_board.checkmate())
		{
			progress({.best_move = move});
			return;
		}
		else
			candidates.push_back(move);
	}

	assert(!candidates.empty());
	AnalysisResult r;
	r.best_move = candidates[rng() % candidates.size()];
	progress(r);
}

int play_game(Engine &white, Engine &black)
{
	auto board = Board::startpos();
	int result = 0;
	for (int halfmove = 0;; ++halfmove)
	{
		if (board.draw())
			break;

		if (board.checkmate())
		{
			result = board.color_to_move == Color::White ? -1 : 1;
			break;
		}

		// Chess doesnt have a fixed 200-moves-rule of course, but this is still
		// useful for overly defensive or random engines.
		if (halfmove >= 400)
		{
			// fmt::print("200 moves without result -> draw\n");
			break;
		}

		auto r = board.color_to_move == Color::White ? white.think(board)
		                                             : black.think(board);
		// fmt::print(" {}", r.best_move);
		board.make_move(r.best_move);
	}
	// fmt::print("\nresult = {}\n", result);
	return result;
}

MatchResult play_match(Engine &left, Engine &right, int games)
{
	using Clock = std::chrono::steady_clock;
	using namespace std::chrono_literals;
	auto last_print = Clock::now();

	MatchResult result;
	for (int i = 0; i < games; ++i)
	{
		int game_result;
		if (i % 2 == 0)
			game_result = play_game(left, right);
		else
			game_result = -play_game(right, left);

		if (game_result > 0)
			++result.left_wins;
		else if (game_result < 0)
			++result.right_wins;
		else
			++result.draws;

		if (Clock::now() - last_print > 1s)
		{
			last_print = Clock::now();
			fmt::print("left={}, right={}, draws={}\n", result.left_wins,
			           result.right_wins, result.draws);
		}
	}

	fmt::print("left={}, right={}, draws={}\n", result.left_wins,
	           result.right_wins, result.draws);

	return result;
}

std::unique_ptr<Engine> make_engine(std::string_view name)
{
	if (name == "random")
		return std::make_unique<RandomEngine>();
	else if (name == "mate-in-one")
		return std::make_unique<MateInOneEngine>();
	else if (name.find(".json") != std::string_view::npos)
	{
		auto json = util::Json::parse_file(name);

		auto eval = std::make_unique<LinearEvaluator>(json["eval"]);
		auto engine = std::make_unique<NegamaxEngine>(std::move(eval));
		engine->set_depth_limit(json["depth_limit"].get<int>());
		engine->set_beta(json["beta"].get<double>());
		return engine;
	}
	else
		throw std::runtime_error("unknown engine");
}

} // namespace metis
