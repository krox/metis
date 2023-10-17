#pragma once

// Helper functions for bitboard manipulation.
// Bitboards are 64-bit integers, each bit representing a square on the board.
// Also the movement patterns of pieces are here, which form the basis for move
// generation.

#include <bit>
#include <cstdint>
#include <string>
#include <string_view>

namespace metis {

enum class Bitboard : uint64_t
{
	file_A = 0x0101010101010101ULL,
	file_B = 0x0202020202020202ULL,
	file_C = 0x0404040404040404ULL,
	file_D = 0x0808080808080808ULL,
	file_E = 0x1010101010101010ULL,
	file_F = 0x2020202020202020ULL,
	file_G = 0x4040404040404040ULL,
	file_H = 0x8080808080808080ULL,

	rank_1 = 0xFF,
	rank_2 = 0xFF00,
	rank_3 = 0xFF0000,
	rank_4 = 0xFF000000,
	rank_5 = 0xFF00000000,
	rank_6 = 0xFF0000000000,
	rank_7 = 0xFF000000000000,
	rank_8 = 0xFF00000000000000,

	all = 0xFFFFFFFFFFFFFFFFULL,
	none = 0ULL,
	center4 = 66229406269440,    // central 4x4 squares
	center6 = 35604928818740736, // central 6x6 squares
};

inline constexpr Bitboard operator&(Bitboard a, Bitboard b)
{
	return Bitboard(uint64_t(a) & uint64_t(b));
}
inline constexpr Bitboard operator|(Bitboard a, Bitboard b)
{
	return Bitboard(uint64_t(a) | uint64_t(b));
}
inline constexpr Bitboard operator^(Bitboard a, Bitboard b)
{
	return Bitboard(uint64_t(a) ^ uint64_t(b));
}
inline constexpr Bitboard operator~(Bitboard a)
{
	return Bitboard(~uint64_t(a));
}

inline constexpr Bitboard &operator&=(Bitboard &a, Bitboard b)
{
	return a = a & b;
}
inline constexpr Bitboard &operator|=(Bitboard &a, Bitboard b)
{
	return a = a | b;
}
inline constexpr Bitboard &operator^=(Bitboard &a, Bitboard b)
{
	return a = a ^ b;
}

inline constexpr Bitboard file(int f)
{
	return Bitboard(uint64_t(Bitboard::file_A) << f);
}
inline constexpr Bitboard rank(int r)
{
	return Bitboard(uint64_t(Bitboard::rank_1) << (r * 8));
}
inline constexpr Bitboard square(int s) { return Bitboard(uint64_t(1) << s); }
inline constexpr int first_set(Bitboard b)
{
	return std::countr_zero(uint64_t(b));
}

inline constexpr bool any(Bitboard b) { return uint64_t(b) != 0; }
inline constexpr bool none(Bitboard b) { return uint64_t(b) == 0; }
inline constexpr int popcount(Bitboard b) { return std::popcount(uint64_t(b)); }

// returns the lowest set bit of a bitboard and clears it. usually for a loop
//     for(uint64_t sq; (sq = pop_lsb(bb)); ) { ... }
inline constexpr Bitboard pop_lsb(Bitboard &bb)
{
	uint64_t lsb = uint64_t(bb) & -uint64_t(bb);
	bb &= Bitboard(uint64_t(bb) - 1);
	return Bitboard(lsb);
}

// shift bitboards in the four cardinal directions, taking care of wraparound
inline constexpr Bitboard up(Bitboard bb)
{
	return Bitboard(uint64_t(bb) << 8);
}
inline constexpr Bitboard down(Bitboard bb)
{
	return Bitboard(uint64_t(bb) >> 8);
}
inline constexpr Bitboard right(Bitboard bb)
{
	return Bitboard(uint64_t(bb & ~Bitboard::file_H) << 1);
}
inline constexpr Bitboard left(Bitboard bb)
{
	return Bitboard(uint64_t(bb & ~Bitboard::file_A) >> 1);
}

// NOTE: the "attacks" include occupied squares regardless of color, so both
//       captures and potential guarding moves. Mask it ~own_pieces to get only
//       actual moves

inline constexpr Bitboard white_pawn_attacks(Bitboard pawns)
{
	auto attacks = Bitboard::none;
	attacks |= left(up(pawns));
	attacks |= right(up(pawns));
	return attacks;
}

inline constexpr Bitboard black_pawn_attacks(Bitboard pawns)
{
	auto attacks = Bitboard::none;
	attacks |= left(down(pawns));
	attacks |= right(down(pawns));
	return attacks;
}

inline constexpr Bitboard white_pawn_moves(Bitboard pawns, Bitboard occupied)
{
	auto moves = Bitboard::none;
	moves |= up(pawns);
	moves |= up(up(pawns & Bitboard::rank_2) & ~occupied);
	return moves & ~occupied;
}

inline constexpr Bitboard black_pawn_moves(Bitboard pawns, Bitboard occupied)
{
	auto moves = Bitboard::none;
	moves |= down(pawns);
	moves |= down(down(pawns & Bitboard::rank_7) & ~occupied);
	return moves & ~occupied;
}

inline constexpr Bitboard pawn_attacks(Bitboard pawns, bool white)
{
	return white ? white_pawn_attacks(pawns) : black_pawn_attacks(pawns);
}
inline constexpr Bitboard pawn_moves(Bitboard pawns, Bitboard occupied,
                                     bool white)
{
	return white ? white_pawn_moves(pawns, occupied)
	             : black_pawn_moves(pawns, occupied);
}

inline constexpr Bitboard knight_attacks(Bitboard knights)
{
	auto attacks = Bitboard::none;
	attacks |= up(up(left(knights)));
	attacks |= up(up(right(knights)));
	attacks |= down(down(left(knights)));
	attacks |= down(down(right(knights)));
	attacks |= left(left(up(knights)));
	attacks |= left(left(down(knights)));
	attacks |= right(right(up(knights)));
	attacks |= right(right(down(knights)));
	return attacks;
}

inline constexpr Bitboard bishop_attacks(Bitboard bishops, Bitboard occupied)
{
	auto attacks = Bitboard::none;
	for (Bitboard tmp = up(left(bishops)); any(tmp);
	     tmp = up(left(tmp & ~occupied)))
		attacks |= tmp;
	for (Bitboard tmp = up(right(bishops)); any(tmp);
	     tmp = up(right(tmp & ~occupied)))
		attacks |= tmp;
	for (Bitboard tmp = down(left(bishops)); any(tmp);
	     tmp = down(left(tmp & ~occupied)))
		attacks |= tmp;
	for (Bitboard tmp = down(right(bishops)); any(tmp);
	     tmp = down(right(tmp & ~occupied)))
		attacks |= tmp;
	return attacks;
}

inline constexpr Bitboard rook_attacks(Bitboard rooks, Bitboard occupied)
{
	auto attacks = Bitboard::none;
	for (Bitboard tmp = up(rooks); any(tmp); tmp = up(tmp & ~occupied))
		attacks |= tmp;
	for (Bitboard tmp = down(rooks); any(tmp); tmp = down(tmp & ~occupied))
		attacks |= tmp;
	for (Bitboard tmp = left(rooks); any(tmp); tmp = left(tmp & ~occupied))
		attacks |= tmp;
	for (Bitboard tmp = right(rooks); any(tmp); tmp = right(tmp & ~occupied))
		attacks |= tmp;
	return attacks;
}

// NOTE: no queen_attacks(), just use bishop_attacks() | rook_attacks()

// without castling
inline constexpr Bitboard king_attacks(Bitboard kings)
{
	auto attacks = Bitboard::none;
	attacks |= up(kings);
	attacks |= down(kings);
	attacks |= left(kings);
	attacks |= right(kings);
	attacks |= up(left(kings));
	attacks |= up(right(kings));
	attacks |= down(left(kings));
	attacks |= down(right(kings));
	return attacks;
}

Bitboard bitboard(std::string_view);
std::string to_string(Bitboard);

} // namespace metis
