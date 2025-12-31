#pragma once

#include "metis/board.h"
#include "metis/move_generator.h"
#include "util/functional.h"
#include "util/random.h"
#include <cassert>
#include <stop_token>

namespace metis {

struct AnalysisResult
{
	Move best_move;
	// TODO: score, alternative_moves, PV, depth, nodes, time, ...
};

// Most engines dont produce a single result, but a sequence of ever improving
// results (e.g. iterative deepening). Thus, it is reported to the caller via a
// callback instead of a direct return value.
using ProgressCallback = util::function_view<void(AnalysisResult const &)>;

// base class for all engines
class Engine
{
  protected:
	util::xoshiro256 rng;

  public:
	// Analyze a position and report the result via the callback. Might call
	// progress multiple times, whenever a better result is found (e.g. via
	// iterative deepening). Stops using the stop_token or when some limit is
	// reached (max_time, max_depth, max_nodes, ...)
	virtual void think(Board const &board, ProgressCallback progress,
	                   std::stop_token stoken) = 0;

	// simplified version of think() that just returns the final result
	AnalysisResult think(Board const &board, std::stop_token stoken = {})
	{
		AnalysisResult result;
		think(board, [&](AnalysisResult const &r) { result = r; }, stoken);
		return result;
	}

	virtual std::unique_ptr<Engine> clone() const = 0;

	virtual ~Engine() = default;

	void seed(uint64_t seed) { rng.seed(seed); }
	void seed(std::string_view seed) { rng.seed(seed); }
	void seed() { rng.seed(std::random_device{}()); }
};

// trivial engine that plays randomly
class RandomEngine final : public Engine
{
  public:
	void think(Board const &board, ProgressCallback progress,
	           std::stop_token stoken) override;

	std::unique_ptr<Engine> clone() const override
	{
		return std::make_unique<RandomEngine>(*this);
	}
};

// trivial engine that detects mate in one, and otherwise plays randomly
class MateInOneEngine final : public Engine
{
  public:
	void think(Board const &board, ProgressCallback progress,
	           std::stop_token stoken) override;

	std::unique_ptr<Engine> clone() const override
	{
		return std::make_unique<MateInOneEngine>(*this);
	}
};

// takes mate in one or random move. Prefers captures to non-captures
class CaptureEngine final : public Engine
{
  public:
	void think(Board const &board, ProgressCallback progress,
	           std::stop_token stoken) override;

	std::unique_ptr<Engine> clone() const override
	{
		return std::make_unique<CaptureEngine>(*this);
	}
};

// play a single game between two engines
int play_game(Engine &white, Engine &black, bool verbose = false);

struct MatchResult
{
	int left_wins = 0;
	int right_wins = 0;
	int draws = 0;
};

// play a multi-game match between two engines
MatchResult play_match(Engine &white, Engine &black, int games);

// create an engine. "name" is either the name of a builtin engine, or a
// filename of a description in json format.
std::unique_ptr<Engine> make_engine(std::string_view name);

} // namespace metis
