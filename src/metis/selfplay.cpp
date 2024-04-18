#include "metis/selfplay.h"
#include "metis/engine.h"
#include "util/io.h"

namespace metis {
namespace {

struct Options
{
	std::string engine = "mate-in-one";
	std::string filename;
	bool allow_overwrite = false;
	bool verbose = false;
	int max_plies = 400;
	int64_t count = 1000;
};

void run_selfplay_command(Options opt)
{
	auto engine = make_engine(opt.engine);

	util::File file;
	if (!opt.filename.empty())
	{
		file = util::File::create(opt.filename, opt.allow_overwrite);
		file.write(uint64_t(13320649049585973946ULL));
		file.write(int32_t(opt.count));
	}

#pragma omp parallel for schedule(dynamic, 1) shared(engine)
	for (int64_t iter = 0; iter < opt.count; ++iter)
	{
		auto board = Board::startpos();
		std::vector<Move> moves;
		int32_t result = 0;
		int halfmoves = 0;
		for (;; halfmoves++)
		{
			if (board.checkmate())
			{
				result = board.color_to_move == Color::White ? -1 : 1;
				break;
			}
			if (board.draw() || halfmoves >= opt.max_plies)
				break;

			auto move = engine->think(board).best_move;
			board.make_move(move);
			moves.push_back(move);
		}

#pragma omp critical
		{
			if (opt.verbose)
			{
				fmt::print("game {}/{}: ", iter + 1, opt.count);
				for (auto move : moves)
					fmt::print("{} ", move);
				fmt::print("result {}\n", result);
			}
			else
			{
				fmt::print("game {}/{}: result {}\n", iter + 1, opt.count,
				           result);
			}

			if (file)
			{
				file.write(int32_t(result));
				file.write(int32_t(moves.size()));
				file.write(moves.data(), moves.size());
			}
		}
	}
}
} // namespace

void setup_selfplay_command(CLI::App &app)
{
	auto opt = std::make_shared<Options>();

	app.add_option("--engine", opt->engine, "Engine to self-play")->required();
	app.add_option("-n,--count", opt->count, "Number of games to play");
	app.add_option("-m,--max-plies", opt->max_plies,
	               "Max plies before draw is declared");
	app.add_option("--filename", opt->filename, "Output filename");
	app.add_flag("--force", opt->allow_overwrite,
	             "Allow overwriting existing files");
	app.add_flag("--verbose", opt->verbose, "Print games to stdout");

	app.callback([opt]() { run_selfplay_command(*opt); });
}
} // namespace metis