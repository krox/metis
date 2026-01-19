#pragma once

#include "metis/engine.h"
#include "metis/evaluator.h"
#include "util/json.h"
#include <cstdint>
#include <memory>
#include <optional>

namespace metis {

// On scores:
//   * positive score means good for the side to move
//   * scores use 'int' for convenience, but actually everything is
//     scaled/clamped to fit into int16_t
//   * scores from the evaluator are clamped by the engine to [-10'000, 10'000].
//     Everything outside this range is for sure win/loss:
//       * +-(31'000-k) = mate in k
//       * -32'000 = illegal position

// minimax with alpha-beta pruning
class NegamaxEngine final : public Engine
{
  public:
	struct Options
	{
		// enable/disable quiescence search
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

	// these are reset at the start of each '.think()' call
	int64_t node_count_ = 0;
	using Clock = std::chrono::steady_clock;
	Clock::time_point start_time_;
	int64_t node_limit_ = INT64_MAX;
	int time_limit_ = INT_MAX; // milliseconds
	std::stop_token stoken_;

	// recursive search function.
	//   * If search was interrupted (time/node limit), returns std::nullopt.
	//   * if depth==0, does quiescence search.
	//   * if result is outside [alpha, beta], it is a lower/upper bound instead
	//     of an exact score.
	//   * future: due to caching, the reult might be somewhat unstable. In
	//     particular if a higher-depth result from cache might always be used
	//     in place of a lower-depth search.
	std::optional<int> search(Board const &, int depth, int alpha, int beta);

	// score 'board' assuming 'move' is played next.
	//   * typically equal to -search(board after move, ...), but also adjusts
	//     mate-scores.
	//   * future: might be correct place to check for repetitions?
	std::optional<int> search(Board board, Move move, int depth, int alpha,
	                          int beta);

  public:
	explicit NegamaxEngine(std::shared_ptr<Evaluator> const &evaluator,
	                       Options const &opts);

	virtual ~NegamaxEngine() = default;
	std::unique_ptr<Engine> clone() const override
	{
		return std::make_unique<NegamaxEngine>(*this);
	}

	void think(Board const &, ProgressCallback, std::stop_token,
	           int time_limit = INT_MAX) override;
};

} // namespace metis
