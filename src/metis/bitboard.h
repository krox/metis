#pragma once

// helper functions for bitboard manipulation

#include <cstdint>

namespace metis {
namespace bb {

static constexpr uint64_t file_A = 0x0101010101010101ULL;
static constexpr uint64_t file_B = file_A << 1;
static constexpr uint64_t file_C = file_A << 2;
static constexpr uint64_t file_D = file_A << 3;
static constexpr uint64_t file_E = file_A << 4;
static constexpr uint64_t file_F = file_A << 5;
static constexpr uint64_t file_G = file_A << 6;
static constexpr uint64_t file_H = file_A << 7;
inline constexpr uint64_t file(int f) { return file_A << f; }

static constexpr uint64_t rank_1 = 0xFF;
static constexpr uint64_t rank_2 = rank_1 << 8;
static constexpr uint64_t rank_3 = rank_1 << 16;
static constexpr uint64_t rank_4 = rank_1 << 24;
static constexpr uint64_t rank_5 = rank_1 << 32;
static constexpr uint64_t rank_6 = rank_1 << 40;
static constexpr uint64_t rank_7 = rank_1 << 48;
static constexpr uint64_t rank_8 = rank_1 << 56;
inline constexpr uint64_t rank(int r) { return rank_1 << (r * 8); }

// returns the lowest set bit of a bitboard and clears it. usually for a loop
//     for(uint64_t sq; (sq = pop_lsb(bb)); ) { ... }
inline uint64_t pop_lsb(uint64_t &bb)
{
	uint64_t lsb = bb & -bb;
	bb &= bb - 1;
	return lsb;
}

inline uint64_t up(uint64_t bb) { return bb << 8; }
inline uint64_t down(uint64_t bb) { return bb >> 8; }
inline uint64_t right(uint64_t bb) { return (bb & ~file_H) << 1; }
inline uint64_t left(uint64_t bb) { return (bb & ~file_A) >> 1; }

// NOTE: the "attacks" include occupied squares regardless of color, so both
//       captures and potential guarding moves. Mask it ~own_pieces to get only
//       actual moves

inline uint64_t white_pawn_attacks(uint64_t pawns)
{
	uint64_t attacks = 0;
	attacks |= left(up(pawns));
	attacks |= right(up(pawns));
	return attacks;
}

inline uint64_t black_pawn_attacks(uint64_t pawns)
{
	uint64_t attacks = 0;
	attacks |= left(down(pawns));
	attacks |= right(down(pawns));
	return attacks;
}

inline uint64_t white_pawn_moves(uint64_t pawns, uint64_t occupied)
{
	uint64_t moves = 0;
	moves |= up(pawns);
	moves |= up(up(pawns & rank_2) & ~occupied);
	return moves & ~occupied;
}

inline uint64_t black_pawn_moves(uint64_t pawns, uint64_t occupied)
{
	uint64_t moves = 0;
	moves |= down(pawns);
	moves |= down(down(pawns & rank_7) & ~occupied);
	return moves & ~occupied;
}

inline uint64_t knight_attacks(uint64_t knights)
{
	uint64_t attacks = 0;
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

inline uint64_t bishop_attacks(uint64_t bishops, uint64_t occupied)
{
	uint64_t attacks = 0;
	uint64_t tmp = (bishops & ~file_A) << 7;
	for (int i = 0; i < 7; ++i)
	{
		attacks |= tmp;
		tmp &= ~occupied;
		tmp = (tmp & ~file_A) << 7;
	}
	tmp = (bishops & ~file_H) << 9;
	for (int i = 0; i < 7; ++i)
	{
		attacks |= tmp;
		tmp &= ~occupied;
		tmp = (tmp & ~file_H) << 9;
	}
	tmp = (bishops & ~file_H) >> 7;
	for (int i = 0; i < 7; ++i)
	{
		attacks |= tmp;
		tmp &= ~occupied;
		tmp = (tmp & ~file_H) >> 7;
	}
	tmp = (bishops & ~file_A) >> 9;
	for (int i = 0; i < 7; ++i)
	{
		attacks |= tmp;
		tmp &= ~occupied;
		tmp = (tmp & ~file_A) >> 9;
	}
	return attacks;
}

inline uint64_t rook_attacks(uint64_t rooks, uint64_t occupied)
{
	uint64_t attacks = 0;
	uint64_t tmp = rooks << 8;
	for (int i = 0; i < 7; ++i)
	{
		attacks |= tmp;
		tmp &= ~occupied;
		tmp = tmp << 8;
	}
	tmp = (rooks & ~file_H) << 1;
	for (int i = 0; i < 7; ++i)
	{
		attacks |= tmp;
		tmp &= ~occupied;
		tmp = (tmp & ~file_H) << 1;
	}
	tmp = (rooks & ~file_A) >> 1;
	for (int i = 0; i < 7; ++i)
	{
		attacks |= tmp;
		tmp &= ~occupied;
		tmp = (tmp & ~file_A) >> 1;
	}
	tmp = rooks >> 8;
	for (int i = 0; i < 7; ++i)
	{
		attacks |= tmp;
		tmp &= ~occupied;
		tmp = tmp >> 8;
	}
	return attacks;
}

// NOTE: no queen_attacks(), just use bishop_attacks() | rook_attacks()

// without castling
inline uint64_t king_attacks(uint64_t kings)
{
	uint64_t attacks = 0;
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

} // namespace bb
} // namespace metis
