#include "CLI/CLI.hpp"
#include "fmt/format.h"
#include "metis/board.h"
#include "metis/engine.h"
#include "metis/evaluator.h"
#include "metis/training.h"

using namespace metis;

int main(int argc, char **argv)
{
	// general options
	std::string fen = starting_fen;
	std::string initial_moves = "";
	bool show_position = false;
	std::string seed = "";

	// peft options
	int depth = -1;

	// match option
	std::string left_engine;
	std::string right_engine;

	// CLI11 app
	auto app = CLI::App{"Metis"};
	app.require_subcommand(1);
	app.add_option("--fen", fen, "FEN");
	app.add_option("--moves", initial_moves, "Initial moves");
	app.add_flag("--show-position", show_position, "Show initial position");

	auto perft = app.add_subcommand("perft", "Perft");
	perft->add_option("-d,--depth", depth, "Depth");

	auto match = app.add_subcommand(
	    "match", "run a multi-game match between two engines");
	match->add_option("left_engine", left_engine, "Left engine");
	match->add_option("right_engine", right_engine, "Right engine");

	auto train = app.add_subcommand(
	    "train", "optimize weights of a evaluation function using self-play");

	// parse
	CLI11_PARSE(app, argc, argv);

	// set up initial position
	auto state = GameState(fen);
	for (auto move : util::split_white(initial_moves))
		state.push_move(Move(move));
	if (show_position)
		state.board.print();

	if (perft->parsed())
	{

		if (depth == -1)
		{
			for (depth = 1;; depth++)
				fmt::print("{}: {}\n", depth, state.perft(depth));
		}
		else
			state.perft_ex(depth);
	}
	else if (match->parsed())
	{
		auto left = make_engine(left_engine);
		auto right = make_engine(right_engine);
		left->seed(fmt::format("{}_left", seed));
		right->seed(fmt::format("{}_right", seed));
		play_match(*left, *right, 100000);
	}
	else if (train->parsed())
	{
		auto engine = make_engine("mate-in-one");

		LinearEvaluator eval;

		eval.add_term({.bb = bb::all, .pt = PieceType::Pawn, .score = 0});
		eval.add_term({.bb = bb::all, .pt = PieceType::Knight, .score = 0});
		eval.add_term({.bb = bb::all, .pt = PieceType::Bishop, .score = 0});
		eval.add_term({.bb = bb::all, .pt = PieceType::Rook, .score = 0});
		eval.add_term({.bb = bb::all, .pt = PieceType::Queen, .score = 0});

		run_training(*engine, eval);
	}
}