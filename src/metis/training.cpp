#include "metis/training.h"
#include "Eigen/Dense"
#include "util/io.h"

namespace metis {
void run_training(std::string_view games_filename, LinearEvaluator &eval)
{
	// F = features (one row per game)
	// R = result (single column)
	// W = weights (single column)
	// want to minimize: | F  W - R |^2
	// solution via normal equations: W = (F^T F)^-1 F^T r
	// accumulate M = F^T F and V = F^T R while reading games
	size_t nfeat = eval.terms().size();
	Eigen::MatrixXf M = Eigen::MatrixXf::Zero(nfeat, nfeat);
	Eigen::VectorXf V = Eigen::VectorXf::Zero(nfeat);
	Eigen::VectorXf f = Eigen::VectorXf::Zero(nfeat);

	auto file = util::File::open(games_filename);
	uint64_t magic;
	file.read(magic);
	if (magic != 13320649049585973946ULL)
		throw std::runtime_error("Invalid magic number in file");
	int32_t count;
	file.read(count);

	std::vector<Move> moves;

	for (int iter = 0; iter < count; ++iter)
	{
		int32_t result, nmoves;

		file.read(result);
		file.read(nmoves);
		moves.resize(nmoves);
		file.read(moves.data(), moves.size());

		auto board = Board::startpos();
		for (auto move : moves)
		{
			board.make_move(move);

			eval.get_features(board, f);
			M += f * f.transpose();
			V += result * f;
		}

		// float weight = 1.0 / nmoves;
		// M += weight * M_tmp;
		// V += (weight * result) * V_tmp;

		if ((iter + 1) % 100 == 0)
		{
			Eigen::VectorXf W = M.inverse() * V;
			eval.set_weights(W);
			fmt::print("iter={}\n{:h}\n", iter + 1, to_json(eval));
		}
	}
}

namespace {
struct Options
{
	std::string filename;
	std::string evaluator;
};

void run_train_command(Options opt)
{

	auto json = util::Json::parse_file(opt.evaluator);
	auto eval = std::make_unique<LinearEvaluator>(json["eval"]);

	run_training(opt.filename, *eval);
}

} // namespace

void setup_train_command(CLI::App &app)
{
	auto opt = std::make_shared<Options>();

	app.add_option("--games", opt->filename, "file to read games from")
	    ->required();
	app.add_option("--evaluator", opt->evaluator, "Evaluator to train")
	    ->required();

	app.callback([opt]() { run_train_command(*opt); });
}
} // namespace metis