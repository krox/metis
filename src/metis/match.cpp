#include "metis/match.h"
#include "metis/engine.h"
#include "util/io.h"
#include <random>

namespace metis {
namespace {

struct Options
{
	std::string left_engine, right_engine;
	int ngames = 100;
};

void run_match_command(Options opt)
{
	auto left = make_engine(opt.left_engine);
	auto right = make_engine(opt.right_engine);
	auto seed = std::random_device{}();
	left->seed(fmt::format("{}_left", seed));
	right->seed(fmt::format("{}_right", seed));
	if (opt.ngames == 1)
		play_game(*left, *right, true);
	else
		play_match(*left, *right, opt.ngames);
}
} // namespace

void setup_match_command(CLI::App &app)
{
	auto opt = std::make_shared<Options>();

	app.add_option("left", opt->left_engine, "left engine")->required();
	app.add_option("right", opt->right_engine, "right engine")->required();
	app.add_option("-n,--count", opt->ngames, "Number of games to play");

	app.callback([opt]() { run_match_command(*opt); });
}
} // namespace metis