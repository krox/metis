#pragma once

#include "metis/bitboard.h"
#include "util/error.h"
#include "util/string.h"
#include "util/vector.h"
#include <array>
#include <cassert>

namespace metis {

enum class CastlingRights : uint8_t
{
	None = 0,
	WhiteKingSide = 1,
	WhiteQueenSide = 2,
	BlackKingSide = 4,
	BlackQueenSide = 8,
	White = WhiteKingSide | WhiteQueenSide,
	Black = BlackKingSide | BlackQueenSide,
	All = 15,
};

// Maybe we should have separate enums (bitfields?) for color/piecetype/piece.
// But for now, this seems very convenient.

enum class PieceType : uint8_t
{
	Pawn = 2,
	Knight,
	Bishop,
	Rook,
	Queen,
	King,
};

enum class Piece : uint8_t
{
	Empty = 0,

	BlackPawn = uint8_t(PieceType::Pawn),
	BlackKnight,
	BlackBishop,
	BlackRook,
	BlackQueen,
	BlackKing,

	WhitePawn = uint8_t(PieceType::Pawn) + 8,
	WhiteKnight,
	WhiteBishop,
	WhiteRook,
	WhiteQueen,
	WhiteKing,

	PieceTypeMask = 7,
	ColorMask = 8,
};

#define ENABLE_BIT_OPS(T)                                                      \
	constexpr bool operator!(T a)                                              \
	{                                                                          \
		return !(uint8_t(a));                                                  \
	}                                                                          \
	constexpr T operator~(T a)                                                 \
	{                                                                          \
		return T(~uint8_t(a));                                                 \
	}                                                                          \
	constexpr T operator|(T a, T b)                                            \
	{                                                                          \
		return T(uint8_t(a) | uint8_t(b));                                     \
	}                                                                          \
	constexpr T operator&(T a, T b)                                            \
	{                                                                          \
		return T(uint8_t(a) & uint8_t(b));                                     \
	}                                                                          \
	constexpr T &operator|=(T &a, T b)                                         \
	{                                                                          \
		a = a | b;                                                             \
		return a;                                                              \
	}                                                                          \
	constexpr T &operator&=(T &a, T b)                                         \
	{                                                                          \
		a = a & b;                                                             \
		return a;                                                              \
	}

// ENABLE_BIT_OPS(Piece)
ENABLE_BIT_OPS(CastlingRights)
ENABLE_BIT_OPS(PieceType)
ENABLE_BIT_OPS(Piece)
#undef ENABLE_BIT_OPS

inline PieceType piecetype(Piece p)
{
	return PieceType(p & Piece::PieceTypeMask);
}

// NOTE: empty squares are neither black nor white
inline constexpr bool is_white(Piece p)
{
	return Piece::WhitePawn <= p && p <= Piece::WhiteKing;
}
inline constexpr bool is_black(Piece p)
{
	return Piece::BlackPawn <= p && p <= Piece::BlackKing;
}

inline constexpr Piece make_piece(PieceType pt, bool white)
{
	return Piece(uint8_t(pt) | (white ? 8 : 0));
}

// convert piece to/from char in standard notation, e.g. 'P' for white pawn
char piece_to_char(Piece);
Piece char_to_piece(char);

// FEN string for the starting position of standard chess
inline const std::string starting_fen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

struct Move
{
	uint8_t from : 6 = 0;
	uint8_t promotion : 2 = 0; // 0: knight, 1: bishop, 2: rook, 3: queen
	uint8_t to : 6 = 0;
	uint8_t special : 2 = 0; // 0: normal, 1: e.p., 2: castling, 3: promotion

	Move() = default;
	Move(int f, int t) : from(f), to(t) {}
	static Move ep(int f, int t)
	{
		Move m{f, t};
		m.special = 1;
		return m;
	}
	static Move castle(int f, int t)
	{
		Move m{f, t};
		m.special = 2;
		return m;
	}

	Move(std::string_view s)
	{
		assert(4 <= s.size() && s.size() <= 5);
		assert('a' <= s[0] && s[0] <= 'h');
		assert('1' <= s[1] && s[1] <= '8');
		assert('a' <= s[2] && s[2] <= 'h');
		assert('1' <= s[3] && s[3] <= '8');
		from = (s[0] - 'a') + 8 * (s[1] - '1');
		to = (s[2] - 'a') + 8 * (s[3] - '1');
		if (s.size() == 5)
		{
			special = 3;
			switch (s[4])
			{
			case 'n':
				promotion = 0;
				break;
			case 'b':
				promotion = 1;
				break;
			case 'r':
				promotion = 2;
				break;
			case 'q':
				promotion = 3;
				break;
			default:
				assert(false);
			}
		}
	}

	bool operator==(Move const &) const = default;
};

static_assert(sizeof(Move) == 2);

// represets the state of a game of chess
class Board
{
	std::array<Piece, 64> squares_;
	std::array<uint64_t, 8> bbs_;     // {black, white, pawn, knight, ...}
	std::array<uint64_t, 2> attacks_; // {black, white}

	// updates 'bbs_' bitboards, but not the attack bitboards. (thus private)
	void place_piece(Piece p, int sq);
	void remove_piece(int sq);

	// (re-)compute the attack bitboards
	void compute_attacks();

  public:
	// some special game state to keep track of (TODO: make more private)
	bool white_to_move = true;
	CastlingRights castling_rights = CastlingRights::All;
	uint64_t ep_square = 0; // either 0 or single-bit

	// TODO: halfmove clock, fullmove number

  public:
	// create empty board (which is not a valid position)
	Board()
	{
		squares_.fill(Piece::Empty);
		bbs_.fill(0);
	}

	// parse a FEN string into a board. Throws if the FEN is invalid
	explicit Board(std::string_view fen);

	// access to individual squares (updating bitboards)
	Piece operator[](int sq) const { return squares_[sq]; }

	// bitboard of all pieces of a certain color/piecetype
	uint64_t bb_color(bool white) const { return bbs_[white]; }
	uint64_t bb_piece(PieceType pt, bool white) const
	{
		return bbs_[int(pt)] & bbs_[white];
	}
	uint64_t bb_piece(Piece p) const
	{
		return bb_piece(piecetype(p), is_white(p));
	}
	uint64_t bb_attacks(bool white) const { return attacks_[white]; }

	// check if we have castling right on certain side.
	// NOTE: this does not check if it is actually possible right now (due to
	// attacked/blocked squares), but only whether king+rook have not moved yet.
	bool castling_right_kingside(bool white) const
	{
		if (white)
			return !!(castling_rights & CastlingRights::WhiteKingSide);
		else
			return !!(castling_rights & CastlingRights::BlackKingSide);
	}
	bool castling_right_queenside(bool white) const
	{
		if (white)
			return !!(castling_rights & CastlingRights::WhiteQueenSide);
		else
			return !!(castling_rights & CastlingRights::BlackQueenSide);
	}

	// Some sanity checks on the board, mostly for debugging.
	// e.g: exactly one king per side, no pawns on last rank.
	// Does not check that a board is actually reachable from the starting
	// position. Starting in non-standard position (i.e. Chess960) is allowed.
	bool valid() const;

	// check if the current position is legal, e.g., cant capture enemy king
	bool legal() const;

	// does not check legality of move
	void make_move(Move move);

	// print human-readable board to stdout
	void print() const;
};

// stack of board positions
class GameState
{
	util::vector<Board> stack_;

  public:
	Board board;
	GameState() : board(starting_fen) {}
	explicit GameState(std::string_view fen) : board(fen) {}

	void push_move(Move move)
	{
		stack_.push_back(board);
		board.make_move(move);
	}

	void pop_move()
	{
		assert(!stack_.empty());
		board = stack_.pop_back();
	}

	// counts number of legal games with d plies
	int64_t perft(int depth);
	void perft_ex(int depth);
};

}; // namespace metis

template <> struct fmt::formatter<metis::Move> : formatter<std::string_view>
{
	template <class FormatContext>
	auto format(metis::Move a, FormatContext &ctx)
	{
		std::string s;
		s.push_back('a' + a.from % 8);
		s.push_back('1' + a.from / 8);
		s.push_back('a' + a.to % 8);
		s.push_back('1' + a.to / 8);
		if (a.special == 3)
			s.push_back("nbrq"[a.promotion]);
		return formatter<std::string_view>::format(s, ctx);
	}
};