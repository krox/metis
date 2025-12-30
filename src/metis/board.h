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

enum class PieceType : uint8_t
{
	None = 0,
	// NOTE: starting from 2 means that PieceType and Color do not overlap,
	// which is nice for some low-level bitboard stuff.
	Pawn = 2,
	Knight,
	Bishop,
	Rook,
	Queen,
	King,
};

enum class Color : uint8_t
{
	Black = 0,
	White = 1,
};

inline Color operator-(Color c) { return Color(!uint8_t(c)); }

enum class Piece : uint8_t
{
	Empty = 0,

	BlackPawn = uint8_t(PieceType::Pawn) | uint8_t(Color::Black) << 3,
	BlackKnight,
	BlackBishop,
	BlackRook,
	BlackQueen,
	BlackKing,

	WhitePawn = uint8_t(PieceType::Pawn) | uint8_t(Color::White) << 3,
	WhiteKnight,
	WhiteBishop,
	WhiteRook,
	WhiteQueen,
	WhiteKing,

	PieceTypeMask = 7,
	ColorMask = 8,
};

#define ENABLE_BIT_OPS(T)                                                      \
	constexpr bool operator!(T a) { return !(uint8_t(a)); }                    \
	constexpr T operator~(T a) { return T(~uint8_t(a)); }                      \
	constexpr T operator|(T a, T b) { return T(uint8_t(a) | uint8_t(b)); }     \
	constexpr T operator&(T a, T b) { return T(uint8_t(a) & uint8_t(b)); }     \
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

// note: color is considered undefined for empty squares
inline Color color(Piece p)
{
	return Color(uint8_t(p & Piece::ColorMask) >> 3);
}

inline constexpr Piece make_piece(PieceType pt, Color color)
{
	return Piece(uint8_t(pt) | uint8_t(color) << 3);
}

// convert piece to/from char in standard notation, e.g. 'P' for white pawn
char piece_to_char(Piece);
Piece char_to_piece(char);

// FEN string for the starting position of standard chess
inline const std::string starting_fen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// This is equivalent to a move in standard notation "e2e4" or "e7e8q". E.g. it
// does not generally contain information about the piece that is moved /
// captured, or any special flags for castling / en passant. All of that has to
// be figured out in make_move using the current board state as context.
// TODO: Add an `ExtendedMove` type with all that extra information. It is
// available during move-generation anyway and would be useful for
// move-ordering for example. Also debugging.
struct Move
{
	// Fun-fact: changing from/to from uint16_t to uint8_t increases the size of
	// `Move` from 2 to 3 bytes. Bitfields are weird.
	uint16_t from : 6 = 0;
	uint16_t to : 6 = 0;
	PieceType promotion : 3 = PieceType::None; // None/Knight/Bishop/Rook/Queen

	Move() = default;

	static Move null() { return Move(); }

	Move(int f, int t) : from(f), to(t) {}

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
			switch (s[4])
			{
			case 'n':
				promotion = PieceType::Knight;
				break;
			case 'b':
				promotion = PieceType::Bishop;
				break;
			case 'r':
				promotion = PieceType::Rook;
				break;
			case 'q':
				promotion = PieceType::Queen;
				break;
			default:
				assert(false);
			}
	}

	bool operator==(Move const &) const = default;
};

static_assert(sizeof(Move) == 2);

// represets the state of a game of chess
class Board
{
	std::array<Piece, 64> squares_;
	std::array<Bitboard, 8> bbs_;     // {black, white, pawn, knight, ...}
	std::array<Bitboard, 2> attacks_; // {black, white}

	// updates 'bbs_' bitboards, but not the attack bitboards. (thus private)
	void place_piece(Piece p, int sq);
	void remove_piece(int sq);

	// (re-)compute the attack bitboards
	void compute_attacks();

  public:
	// some special game state to keep track of (TODO: make more private)
	// (default values are for starting position)
	Color color_to_move = Color::White;
	CastlingRights castling_rights = CastlingRights::All;
	Bitboard ep_square = Bitboard::none; // either 0 or single-bit

	// TODO: halfmove clock, fullmove number

  public:
	// create empty board (which is not a valid position)
	Board()
	{
		squares_.fill(Piece::Empty);
		bbs_.fill(Bitboard::none);
	}

	// starting position for normal chess
	static Board startpos();

	// parse an extended fen string, as used in the UCI protocol:
	// startpos | fen <fenstring> | <fenstring> [moves <movelist>]
	// where <fenstring> consists of up to 6 space separated fields:
	//   pieces: for example rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR
	//   side to move: either 'w' or 'b'
	//   castling rights: subset of 'KQkq' or '-' if none
	//   en passant square: i.e. "e3", "d6", or '-' if none
	//   halfmove clock (optional): plies since last capture or pawn move
	//   fullmove number (optional): starts at 1, incremented after black's move
	static Board from_fen(util::Parser &);
	static Board from_fen(std::string_view);

	// access to individual squares (updating bitboards)
	Piece operator[](int sq) const { return squares_[sq]; }

	// bitboard of all pieces of a certain color/piecetype
	Bitboard bb_all_pieces() const
	{
		return bb_color(Color::Black) | bb_color(Color::White);
	}
	Bitboard bb_color(Color color) const { return bbs_[int(color)]; }
	Bitboard bb_piece(PieceType pt, Color color) const
	{
		return bbs_[int(pt)] & bbs_[int(color)];
	}
	Bitboard bb_piece(Piece p) const
	{
		return bb_piece(piecetype(p), color(p));
	}
	Bitboard bb_attacks(Color color) const { return attacks_[int(color)]; }

	// check if we have castling right on certain side.
	// NOTE: this does not check if it is actually possible right now (due to
	// attacked/blocked squares), but only whether king+rook have not moved yet.
	bool castling_right_kingside(Color color) const
	{
		if (color == Color::White)
			return !!(castling_rights & CastlingRights::WhiteKingSide);
		else
			return !!(castling_rights & CastlingRights::BlackKingSide);
	}
	bool castling_right_queenside(Color color) const
	{
		if (color == Color::White)
			return !!(castling_rights & CastlingRights::WhiteQueenSide);
		else
			return !!(castling_rights & CastlingRights::BlackQueenSide);
	}

	// Some sanity checks on the board, mostly for debugging.
	// e.g: exactly one king per side, no pawns on last rank.
	// Does not check that a board is actually reachable from the starting
	// position. Starting in non-standard position (i.e. Chess960) is allowed.
	bool valid() const;

	// check if the current position is legal, e.g., cant capture enemy king.
	// This function is necessary to filter out illegal moves produced by the
	// pseudo-legal move generator.
	bool legal() const;

	// are there any legal moves?
	// (if not, its either checkmate or stalemate)
	bool has_legal_moves() const;

	// no more legal moves, but not in check?
	// TODO: 50-move rule, 3-fold repetition, insufficient material?
	bool draw() const;

	// is current players king in check?
	bool in_check() const;

	// in_check() && no legal moves
	bool checkmate() const;

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
	GameState() : board(Board::startpos()) {}
	explicit GameState(std::string_view fen) : board(Board::from_fen(fen)) {}

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
	auto format(metis::Move a, FormatContext &ctx) const
	{
		std::string s;
		s.push_back('a' + a.from % 8);
		s.push_back('1' + a.from / 8);
		s.push_back('a' + a.to % 8);
		s.push_back('1' + a.to / 8);
		switch (a.promotion)
		{
		case metis::PieceType::None:
			break;
		case metis::PieceType::Knight:
			s.push_back('n');
			break;
		case metis::PieceType::Bishop:
			s.push_back('b');
			break;
		case metis::PieceType::Rook:
			s.push_back('r');
			break;
		case metis::PieceType::Queen:
			s.push_back('q');
			break;
		default:
			assert(false);
		}
		return formatter<std::string_view>::format(s, ctx);
	}
};