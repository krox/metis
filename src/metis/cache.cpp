#include "metis/cache.h"

#include <algorithm>
#include <bit>

namespace metis {

Cache::Cache(size_t mebibytes)
{
	auto n = std::bit_floor(mebibytes * 1024 * 1024 / sizeof(Entry));
	entries_.assign(n, {});
}

void Cache::clear()
{
	for (auto &entry : entries_)
		entry = Entry{};
}

std::optional<Cache::Entry> Cache::probe(uint64_t key) const
{
	if (entries_.empty())
		return {};
	auto tag = uint16_t(key >> 48);
	auto const &entry = entries_[key & (entries_.size() - 1)];
	if (entry.tag != tag)
		return {};
	if (entry.flags == Bound::None)
		return {};
	return entry;
}

void Cache::store(uint64_t key, int depth, int score, Bound bound, Move pv_move)
{
	if (entries_.empty() || bound == Bound::None)
		return;
	assert(0 <= depth && depth <= 255);
	assert(INT16_MIN <= score && score <= INT16_MAX);

	auto tag = uint16_t(key >> 48);
	auto &entry = entries_[key & (entries_.size() - 1)];

	entry.tag = tag;
	entry.score = int16_t(score);
	entry.pv_move = pv_move;
	entry.depth = uint8_t(depth);
	entry.flags = bound;
}

} // namespace metis
