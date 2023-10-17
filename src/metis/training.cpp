#include "metis/training.h"
#include "Eigen/Dense"

namespace metis {
void run_training(Engine &engine, LinearEvaluator &eval)
{
	// F = features (one row per game)
	// R = result (single column)
	// W = weights (single column)
	// want to optimize: | F  W - R |^2
	// solution via normal equations: W = (F^T F)^-1 F^T r
	// accumulate M = F^T F and V = F^T R during self-play
	size_t nfeat = eval.terms().size();
	Eigen::MatrixXf M = Eigen::MatrixXf::Zero(nfeat, nfeat);
	Eigen::VectorXf V = Eigen::VectorXf::Zero(nfeat);

#pragma omp parallel shared(M, V)
	{
		Eigen::VectorXf f = Eigen::VectorXf::Zero(nfeat);
		Eigen::MatrixXf M_tmp = Eigen::MatrixXf::Zero(nfeat, nfeat);
		Eigen::VectorXf V_tmp = Eigen::VectorXf::Zero(nfeat);
#pragma omp for schedule(dynamic, 1)
		for (size_t iter = 0; iter < 100000; ++iter)
		{
			auto board = Board::startpos();
			M_tmp.setZero();
			V_tmp.setZero();
			int result = 0;
			int halfmoves = 0;
			for (;; halfmoves++)
			{
				if (board.checkmate())
				{
					result = board.color_to_move == Color::White ? -1 : 1;
					break;
				}
				if (board.draw() || halfmoves >= 400)
					break;

				auto move = engine.think(board).best_move;
				board.make_move(move);

				eval.get_features(board, f);
				M_tmp += f * f.transpose();
				V_tmp += f;
			}

			float weight = 1.0 / halfmoves;

#pragma omp critical
			{
				M += weight * M_tmp;
				V += (weight * result) * V_tmp;

				if ((iter + 1) % 10 == 0)
				{
					Eigen::VectorXf W = M.inverse() * V;
					eval.set_weights(W);
					fmt::print("iter={}\n{:h}\n", iter + 1, to_json(eval));
				}
			}
		}
	}
}

namespace {
struct Options
{
	std::string engine;
	std::string evaluator;
};

void run_train_command(Options opt)
{
	if (opt.evaluator.empty())
		opt.evaluator = opt.engine;

	auto engine = make_engine(opt.engine);

	auto json = util::Json::parse_file(opt.evaluator);
	auto eval = std::make_unique<LinearEvaluator>(json["eval"]);

	run_training(*engine, *eval);
}

} // namespace

void setup_train_command(CLI::App &app)
{
	auto opt = std::make_shared<Options>();

	app.add_option("engine", opt->engine, "Engine to train")->required();
	app.add_option("evaluator", opt->evaluator, "Evaluator to train");

	app.callback([opt]() { run_train_command(*opt); });
}
} // namespace metis