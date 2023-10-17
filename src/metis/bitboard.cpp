#include "metis/bitboard.h"

#include "fmt/format.h"
#include "util/lexer.h"

namespace metis {
std::string to_string(Bitboard b)
{
	switch (b)
	{
	case Bitboard::none:
		return "none";
	case Bitboard::all:
		return "all";
	case Bitboard::center4:
		return "center4";
	case Bitboard::center6:
		return "center6";
	default:
		return std::to_string(uint64_t(b));
	}
}

Bitboard bitboard(std::string_view s)
{
	if (s == "all")
		return Bitboard::all;
	if (s == "none")
		return Bitboard::none;
	if (s == "center4")
		return Bitboard::center4;
	if (s == "center6")
		return Bitboard::center6;
	return Bitboard(util::parse_int<uint64_t>(s));
}

} // namespace metis