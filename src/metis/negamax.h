#pragma once

#include "metis/engine.h"
#include "metis/evaluator.h"
#include <cstdint>
#include <memory>

namespace metis {

// minimax with alpha-beta pruning
class NegamaxEngine : public Engine
{
	std::unique_ptr<Evaluator> eval_;

	double beta_ = 0.01;

	// These limits are per invocation of `think()`
	int depth_limit_ = 9999999;

	// recursive search function
	int search(Board const &, int depth, int alpha, int beta,
	           std::stop_token const &);
	int search(Board board, Move move, int depth, int alpha, int beta,
	           std::stop_token const &);

  public:
	NegamaxEngine(std::unique_ptr<Evaluator> eval) : eval_(std::move(eval))
	{
		assert(eval_);
	}
	virtual ~NegamaxEngine() = default;

	void set_depth_limit(int depth) { depth_limit_ = depth; }

	void set_beta(double beta) { beta_ = beta; }

	void think(Board const &, ProgressCallback, std::stop_token) override;
};

} // namespace metis
