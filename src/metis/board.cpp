#include "metis/board.h"

#include "metis/move_generator.h"
#include <cassert>

namespace metis {

char piece_to_char(Piece p)
{
	switch (p)
	{
	case Piece::WhitePawn:
		return 'P';
	case Piece::WhiteKnight:
		return 'N';
	case Piece::WhiteBishop:
		return 'B';
	case Piece::WhiteRook:
		return 'R';
	case Piece::WhiteQueen:
		return 'Q';
	case Piece::WhiteKing:
		return 'K';
	case Piece::BlackPawn:
		return 'p';
	case Piece::BlackKnight:
		return 'n';
	case Piece::BlackBishop:
		return 'b';
	case Piece::BlackRook:
		return 'r';
	case Piece::BlackQueen:
		return 'q';
	case Piece::BlackKing:
		return 'k';
	case Piece::Empty:
		return '.';
	default:
		assert(false);
	}
}

Piece char_to_piece(char c)
{
	switch (c)
	{
	case 'P':
		return Piece::WhitePawn;
	case 'N':
		return Piece::WhiteKnight;
	case 'B':
		return Piece::WhiteBishop;
	case 'R':
		return Piece::WhiteRook;
	case 'Q':
		return Piece::WhiteQueen;
	case 'K':
		return Piece::WhiteKing;
	case 'p':
		return Piece::BlackPawn;
	case 'n':
		return Piece::BlackKnight;
	case 'b':
		return Piece::BlackBishop;
	case 'r':
		return Piece::BlackRook;
	case 'q':
		return Piece::BlackQueen;
	case 'k':
		return Piece::BlackKing;
	case '.':
		return Piece::Empty;
	default:
		assert(false);
	}
}

void Board::place_piece(Piece p, int sq)
{
	assert(squares_[sq] == Piece::Empty);
	squares_[sq] = p;
	bbs_[int(color(p))] |= square(sq);
	bbs_[int(piecetype(p))] |= square(sq);
}

void Board::remove_piece(int sq)
{
	auto p = squares_[sq];
	assert(p != Piece::Empty);
	squares_[sq] = Piece::Empty;
	bbs_[int(color(p))] &= ~square(sq);
	bbs_[int(piecetype(p))] &= ~square(sq);
}

void Board::compute_attacks()
{
	auto occupied = bb_all_pieces();

	Bitboard &black = attacks_[int(Color::Black)];
	black = Bitboard::none;
	black |= black_pawn_attacks(bb_piece(Piece::BlackPawn));
	black |= knight_attacks(bb_piece(Piece::BlackKnight));
	black |= bishop_attacks(
	    bb_piece(Piece::BlackBishop) | bb_piece(Piece::BlackQueen), occupied);
	black |= rook_attacks(
	    bb_piece(Piece::BlackRook) | bb_piece(Piece::BlackQueen), occupied);
	black |= king_attacks(bb_piece(Piece::BlackKing));

	Bitboard &white = attacks_[int(Color::White)];
	white = Bitboard::none;
	white |= white_pawn_attacks(bb_piece(Piece::WhitePawn));
	white |= knight_attacks(bb_piece(Piece::WhiteKnight));
	white |= bishop_attacks(
	    bb_piece(Piece::WhiteBishop) | bb_piece(Piece::WhiteQueen), occupied);
	white |= rook_attacks(
	    bb_piece(Piece::WhiteRook) | bb_piece(Piece::WhiteQueen), occupied);
	white |= king_attacks(bb_piece(Piece::WhiteKing));
}

Board Board::startpos() { return from_fen(starting_fen); }

Board Board::from_fen(std::string_view fen)
{
	Board b;

	static constexpr std::string_view msg = "invalid FEN string";
	auto parts = util::split_white(fen);
	util::check(4 <= parts.size() && parts.size() <= 6, msg);

	// parts[0]: pieces on the board
	auto ranks = util::split(parts[0], '/');
	util::check(ranks.size() == 8, msg);
	for (size_t rank = 0; rank < 8; ++rank)
	{
		auto file = 0;
		for (auto c : ranks[rank])
		{
			if ('1' <= c && c <= '8')
				file += c - '0';
			else
			{
				util::check(file < 8, msg);
				b.place_piece(char_to_piece(c), (7 - rank) * 8 + file);
				++file;
			}
		}
		util::check(file == 8, msg);
	}

	// parts[1]: who's turn it is
	util::check(parts[1] == "w" || parts[1] == "b", msg);
	b.color_to_move = parts[1] == "w" ? Color::White : Color::Black;

	// parts[2]: castling rights
	b.castling_rights = CastlingRights::None;
	if (parts[2] != "-")
		for (char c : parts[2])
		{
			if (c == 'K')
				b.castling_rights |= CastlingRights::WhiteKingSide;
			else if (c == 'Q')
				b.castling_rights |= CastlingRights::WhiteQueenSide;
			else if (c == 'k')
				b.castling_rights |= CastlingRights::BlackKingSide;
			else if (c == 'q')
				b.castling_rights |= CastlingRights::BlackQueenSide;
			else
				util::check(false, msg);
		}

	// parts[3]: en passant square
	if (parts[3] != "-")
	{
		util::check(parts[3].size() == 2, msg);
		util::check('a' <= parts[3][0] && parts[3][0] <= 'h', msg);
		util::check('1' <= parts[3][1] && parts[3][1] <= '8', msg);
		b.ep_square = square((parts[3][0] - 'a') + 8 * (parts[3][1] - '1'));
	}

	// parts[4]: halfmove clock (optional)
	// TODO

	// parts[5]: fullmove number (optional)
	// TODO

	util::check(b.valid(), msg);
	b.compute_attacks();
	return b;
}

Board Board::from_uci(std::string_view s)
{
	// find 'moves' keyword
	std::string_view moves;
	if (size_t pos = s.find("moves"); pos != size_t(-1))
	{
		moves = s.substr(pos + 5);
		s = s.substr(0, pos);
	}

	s = util::trim_white(s);
	Board b;
	if (s == "startpos")
		b = startpos();
	else if (s.starts_with("fen"))
		b = from_fen(s.substr(3));
	else
		b = from_fen(s);

	for (auto &token : util::split_white(moves))
		b.make_move(Move(token));
	return b;
}

bool Board::valid() const
{
	int white_kings = 0;
	int black_kings = 0;
	for (size_t i = 0; i < 64; ++i)
	{
		auto p = squares_[i];

		if (p == Piece::WhiteKing)
			++white_kings;
		else if (p == Piece::BlackKing)
			++black_kings;

		if (p == Piece::Empty)
			continue;

		// no pawns in the last rank
		if (p == Piece::WhitePawn && i >= 56)
			return false;
		if (p == Piece::BlackPawn && i < 8)
			return false;
	}
	return white_kings == 1 && black_kings == 1;
}

bool Board::legal() const
{
	return none(bb_piece(PieceType::King, -color_to_move) &
	            bb_attacks(color_to_move));
}

bool Board::has_legal_moves() const
{
	MoveList moves;
	generate_pseudolegal_moves(*this, moves);
	for (auto move : moves)
	{
		Board new_board = *this;
		new_board.make_move(move);
		if (new_board.legal())
			return true;
	}
	return false;
}

bool Board::draw() const { return !in_check() && !has_legal_moves(); }

bool Board::in_check() const
{
	return any(bb_piece(PieceType::King, color_to_move) &
	           bb_attacks(-color_to_move));
}

bool Board::checkmate() const { return in_check() && !has_legal_moves(); }

void Board::make_move(Move move)
{
	auto piece = squares_[move.from];
	bool pawn_move = piecetype(piece) == PieceType::Pawn;
	bool pawn_double_push = pawn_move && abs(move.from - move.to) == 16;
	bool pawn_attack = pawn_move && abs(move.from - move.to) != 8 &&
	                   abs(move.from - move.to) != 16;
	assert(piece != Piece::Empty && "invalid move (from square is empty)");
	assert(move.from != move.to && "invalid move (from and to are the same)");

	// en-passant right for next move
	if (pawn_double_push)
		ep_square = square((move.from + move.to) / 2);
	else
		ep_square = Bitboard::none;

	// remove old piece(s)
	remove_piece(move.from);
	if (squares_[move.to] != Piece::Empty) // normal capture
		remove_piece(move.to);
	else if (pawn_attack) // e.p. capture
	{
		if (color_to_move == Color::White)
			remove_piece(move.to - 8);
		else
			remove_piece(move.to + 8);
	}

	// place new piece
	if (move.promotion != PieceType::None) // promotion
		piece = make_piece(move.promotion, color_to_move);
	place_piece(piece, move.to);

	// move rook when castling
	if (piecetype(piece) == PieceType::King)
	{
		if (move.from == 4 && move.to == 2)
		{
			remove_piece(0);
			place_piece(Piece::WhiteRook, 3);
		}
		else if (move.from == 4 && move.to == 6)
		{
			remove_piece(7);
			place_piece(Piece::WhiteRook, 5);
		}
		else if (move.from == 60 && move.to == 58)
		{
			remove_piece(56);
			place_piece(Piece::BlackRook, 59);
		}
		else if (move.from == 60 && move.to == 62)
		{
			remove_piece(63);
			place_piece(Piece::BlackRook, 61);
		}
	}

	// remove castling rights if rook or king moves or is captured
	if (move.to == 0 || move.from == 0)
		castling_rights &= ~CastlingRights::WhiteQueenSide;
	if (move.to == 7 || move.from == 7)
		castling_rights &= ~CastlingRights::WhiteKingSide;
	if (move.to == 56 || move.from == 56)
		castling_rights &= ~CastlingRights::BlackQueenSide;
	if (move.to == 63 || move.from == 63)
		castling_rights &= ~CastlingRights::BlackKingSide;
	if (move.from == 4)
		castling_rights &= ~CastlingRights::White;
	if (move.from == 60)
		castling_rights &= ~CastlingRights::Black;

	color_to_move = -color_to_move;
	compute_attacks();
}

void Board::print() const
{
	for (int rank = 7; rank >= 0; --rank)
	{
		for (int file = 0; file < 8; ++file)
			fmt::print("{} ", piece_to_char(squares_[rank * 8 + file]));
		fmt::print("\n");
	}
}

int64_t GameState::perft(int depth)
{
	if (depth == 0)
		return 1;
	MoveList moves;
	generate_pseudolegal_moves(board, moves);

	int64_t r = 0;
	for (auto move : moves)
	{
		push_move(move);
		if (board.legal())
			r += perft(depth - 1);
		pop_move();
	}
	return r;
}

// ditto, printing the result human readable
void GameState::perft_ex(int depth)
{
	if (depth == 0)
	{
		fmt::print("Total: 1\n");
		return;
	}

	MoveList moves;
	generate_pseudolegal_moves(board, moves);
	int64_t total = 0;
	for (auto move : moves)
	{
		push_move(move);
		if (board.legal())
		{
			int64_t n = perft(depth - 1);
			total += n;
			fmt::print("{}: {}\n", move, n);
		}
		pop_move();
	}
	fmt::print("Total: {}\n", total);
}
} // namespace metis