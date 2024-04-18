#include "CLI/CLI.hpp"
#include "fmt/format.h"
#include "metis/board.h"
#include "metis/engine.h"
#include "metis/evaluator.h"
#include "metis/selfplay.h"
#include "metis/training.h"
#include "metis/uci.h"

using namespace metis;

int main(int argc, char **argv)
{
	// general options
	std::string fen = starting_fen;
	std::string initial_moves = "";
	bool show_position = false;
	std::string seed = "";
	int ngames = 1000;

	// peft options
	int depth = -1;

	// match option
	std::string left_engine;
	std::string right_engine;

	// CLI11 app
	auto app = CLI::App{"Metis"};
	app.require_subcommand(0, 1);
	app.add_option("--fen", fen, "FEN");
	app.add_option("--moves", initial_moves, "Initial moves");
	app.add_flag("--show-position", show_position, "Show initial position");

	auto perft = app.add_subcommand("perft", "Perft");
	perft->add_option("-d,--depth", depth, "Depth");

	auto match = app.add_subcommand(
	    "match", "run a multi-game match between two engines");
	match->add_option("left_engine", left_engine, "Left engine");
	match->add_option("right_engine", right_engine, "Right engine");
	match->add_option("-n,--games", ngames, "Number of games");

	auto uci = app.add_subcommand("uci", "UCI mode");
	(void)uci;
	auto train = app.add_subcommand(
	    "train", "optimize weights of a evaluation function using self-play");
	setup_train_command(*train);

	auto selfplay = app.add_subcommand(
	    "selfplay",
	    "play games against itself (typically to be used for training later)");
	setup_selfplay_command(*selfplay);

	// parse
	CLI11_PARSE(app, argc, argv);

	return 0;
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
		if (ngames == 1)
			play_game(*left, *right, true);
		else
			play_match(*left, *right, ngames);
	}
	else /*if (uci->parsed())*/
	{
		UCIHandler handler(make_engine("mate-in-one"));
		handler.set_log_file("uci.log");
		handler.run();
	}
}