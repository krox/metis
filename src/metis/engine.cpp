#include "metis/engine.h"

#include "metis/negamax.h"

namespace metis {

void RandomEngine::think(Board const &board, ProgressCallback progress,
                         std::stop_token stoken, int,
                         std::span<uint64_t const> history)
{
	(void)stoken;
	(void)history;

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
                            std::stop_token stoken, int,
                            std::span<uint64_t const> history)
{
	(void)stoken;
	(void)history;

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

void CaptureEngine::think(Board const &board, ProgressCallback progress,
                          std::stop_token stoken, int,
                          std::span<uint64_t const> history)
{
	(void)stoken;
	(void)history;
	MoveList candidates;
	generate_pseudolegal_moves(board, candidates);

	MoveList moves;
	MoveList capture_moves;
	for (auto move : candidates)
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

		if (board[move.to] != Piece::Empty ||
		    (piecetype(board[move.from]) == PieceType::Pawn &&
		     square(move.to) == board.ep_square))
			capture_moves.push_back(move);
		else
			moves.push_back(move);
	}

	if (!capture_moves.empty())
	{
		// take random capture
		progress({.best_move = capture_moves[rng() % capture_moves.size()]});
		return;
	}

	// otherwise random move
	assert(!moves.empty());
	progress({.best_move = moves[rng() % moves.size()]});
}

int play_game(Engine &white, Engine &black, bool verbose)
{
	auto board = Board::startpos();
	util::vector<uint64_t> history;
	history.push_back(board.zobrist());
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

		auto r = board.color_to_move == Color::White
		             ? white.think(board, {}, INT_MAX, history)
		             : black.think(board, {}, INT_MAX, history);
		if (verbose)
		{
			board.print();
			fmt::print(" {}\n", r.best_move);
			fflush(stdout);
		}
		board.make_move(r.best_move);
		history.push_back(board.zobrist());
	}
	if (verbose)
		fmt::print("\nresult = {}\n", result);
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
	// builtin engines
	if (name == "random")
		return std::make_unique<RandomEngine>();
	if (name == "mate-in-one")
		return std::make_unique<MateInOneEngine>();
	if (name == "capture")
		return std::make_unique<CaptureEngine>();
	if (name == "material")
	{
		auto evaluator = std::make_shared<MaterialEvaluator>();
		return std::make_unique<NegamaxEngine>(evaluator,
		                                       NegamaxEngine::Options{});
	}

	// json description
	if (name.find(".json") != std::string_view::npos)
	{
		auto json = util::Json::parse_file(name);

		if (json.at("type").get<std::string>() == "negamax")
		{
			auto evaluator = std::make_shared<LinearEvaluator>(json.at("eval"));
			return std::make_unique<NegamaxEngine>(
			    evaluator, NegamaxEngine::Options(json));
		}
		else
			throw std::runtime_error("unknown engine type in json");
	}
	else
		throw std::runtime_error("unknown engine");
}

} // namespace metis
