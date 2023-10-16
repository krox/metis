#pragma once

#include "metis/board.h"

// forward declare Eigen::VectorXf
namespace Eigen {
template <typename, int, int, int, int, int> class Matrix;
using VectorXf = Matrix<float, -1, 1, 0, -1, 1>;
} // namespace Eigen

namespace metis {

class Evaluator
{
  public:
	Evaluator() = default;
	virtual ~Evaluator() = default;

	// Does not check for checkmate/draw, purely the heurisitc evaluation.
	//    * Score is from whites perspective (positive = good for white)
	//    * +-10'000 = sure win/loss (heuristically)
	//    * values outside [-10'000, 10'000] are possible, but unlikely. Have to
	//      be clamped by the user.
	virtual int evaluate(const Board &board) const = 0;

	// for heuristic move ordering.
	// virtual int16_t evaluate_move(Board const&, Move) = 0
};

// Equivalent to piece-square tables, but written as a linear combination of
//     `popcount(bb_pieces(piece) & mask)`
// Coefficients should be trainable by simple linear regression.
class LinearEvaluator : public Evaluator
{
  public:
	struct Term
	{
		uint64_t bb;
		PieceType pt;
		int score;
	};

  private:
	std::vector<Term> terms_;

  public:
	std::vector<Term> const &terms() const { return terms_; }
	void add_term(Term term) { terms_.push_back(term); }

	// get the features for a given board. Used for training
	void get_features(Board const &board, Eigen::VectorXf &features) const;

	int evaluate(Board const &board) const override;
};

} // namespace metis