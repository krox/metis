#include "metis/evaluator.h"

#include "Eigen/Dense"
#include "metis/board.h"
#include <bit>

namespace metis {

int LinearEvaluator::evaluate(Board const &board) const
{
	int score = 0;
	for (auto term : terms_)
	{
		auto bb_white = board.bb_piece(term.pt, Color::White);
		auto bb_black = board.bb_piece(term.pt, Color::Black);
		score += term.score * (std::popcount(term.bb & bb_white) -
		                       std::popcount(term.bb & bb_black));
	}
	return score;
}

void LinearEvaluator::get_features(Board const &board,
                                   Eigen::VectorXf &features) const
{
	assert(features.size() == (int)terms_.size());

	for (size_t i = 0; i < terms_.size(); ++i)
	{
		auto term = terms_[i];
		auto bb_white = board.bb_piece(term.pt, Color::White);
		auto bb_black = board.bb_piece(term.pt, Color::Black);
		features[i] = std::popcount(term.bb & bb_white) -
		              std::popcount(term.bb & bb_black);
	}
}

} // namespace metis