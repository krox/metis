#include "CLI/CLI.hpp"
#include "fmt/format.h"
#include "metis/board.h"

using namespace metis;

int main(int argc, char **argv)
{
	// command line arguments
	std::string fen = starting_fen;
	int depth = -1;
	std::string initial_moves = "";
	bool show_position = false;

	// CLI11 app
	auto app = CLI::App{"Metis"};
	app.require_subcommand(1);

	auto perft = app.add_subcommand("perft", "Perft");
	perft->add_option("--fen", fen, "FEN");
	perft->add_option("-d,--depth", depth, "Depth");
	perft->add_option("--moves", initial_moves, "Initial moves");
	perft->add_flag("--show-position", show_position, "Show initial position");

	// parse
	CLI11_PARSE(app, argc, argv);

	// run

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
}