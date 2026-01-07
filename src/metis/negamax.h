#pragma once

#include "metis/engine.h"
#include "metis/evaluator.h"
#include "util/json.h"
#include <cstdint>
#include <memory>

namespace metis {

// minimax with alpha-beta pruning
class NegamaxEngine final : public Engine
{
  public:
	struct Options
	{
		// These limits are per invocation of `think()`
		int depth_limit = INT_MAX;
		int node_limit = INT_MAX;
		int time_limit = INT_MAX; // milliseconds

		bool qsearch = true;

		Options() = default;
		Options(util::Json const &json)
		{
			qsearch = json.value<bool>("qsearch", true);
		}
	};

  private:
	Options options_;
	std::shared_ptr<Evaluator> eval_;

	// statistics
	int64_t node_count_ = 0;

	// recursive search function
	int search(Board const &, int depth, int alpha, int beta,
	           std::stop_token const &);
	int search(Board board, Move move, int depth, int alpha, int beta,
	           std::stop_token const &);

  public:
	explicit NegamaxEngine(util::Json const &j);

	virtual ~NegamaxEngine() = default;
	std::unique_ptr<Engine> clone() const override
	{
		return std::make_unique<NegamaxEngine>(*this);
	}

	void think(Board const &, ProgressCallback, std::stop_token,
	           int time_limit = INT_MAX) override;
};

} // namespace metis
