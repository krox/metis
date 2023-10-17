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

	Eigen::VectorXf f = Eigen::VectorXf::Zero(nfeat);
	Eigen::MatrixXf M_tmp = Eigen::MatrixXf::Zero(nfeat, nfeat);
	Eigen::VectorXf V_tmp = Eigen::VectorXf::Zero(nfeat);

	for (size_t iter = 0;; ++iter)
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
		M += weight * M_tmp;
		V += (weight * result) * V_tmp;

		// after 2*nfeat games, we can somewhat assume that M is invertible
		if (iter % 100 == 0 && iter >= 2 * nfeat)
		{
			Eigen::VectorXf W = M.inverse() * V;
			eval.set_weights(W);
			fmt::print("iter={}\n{:h}\n", iter, to_json(eval));
		}
	}
}
} // namespace metis