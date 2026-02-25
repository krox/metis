#pragma once

#include "metis/board.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace metis {

// basic transposition table for caching search results.
//    * entries include score and PV move
//    * 2-byte hash is stored as tag
//    * TODO: buckets with 2-4 entries, and some proper replacement strategy
class Cache
{
  public:
	enum class Bound : uint8_t
	{
		None = 0,
		Exact = 1,
		Lower = 2,
		Upper = 3,
	};

	struct Entry
	{
		uint16_t tag = 0; // high bits of zobrist stored as tag, low bits used
		                  // for index into cache
		int16_t score = 0;
		Move pv_move = Move::null();
		uint8_t depth = 0;
		Bound flags = Bound::None;
	};
	static_assert(sizeof(Entry) == 8);

	explicit Cache(size_t mebibytes = 16);
	void clear();

	std::optional<Entry> probe(uint64_t key) const;
	void store(uint64_t key, int depth, int score, Bound bound, Move pv_move);

  private:
	std::vector<Entry> entries_;
};

} // namespace metis
