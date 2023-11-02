#include "metis/move_generator.h"

#include "metis/bitboard.h"
#include "metis/board.h"

namespace metis {

void generate_pseudolegal_moves(Board const &board, MoveList &moves,
                                bool capture_only)
{
	size_t head = moves.size();

	auto my_color = board.color_to_move;
	auto me = board.bb_color(my_color);
	auto them = board.bb_color(-my_color);
	auto occupied = me | them;
	auto mask = capture_only ? them : ~me;

	auto pawns = board.bb_piece(PieceType::Pawn, my_color);
	auto knights = board.bb_piece(PieceType::Knight, my_color);
	auto bishops = board.bb_piece(PieceType::Bishop, my_color);
	auto rooks = board.bb_piece(PieceType::Rook, my_color);
	auto queens = board.bb_piece(PieceType::Queen, my_color);
	auto kings = board.bb_piece(PieceType::King, my_color);

	auto gen = [&](Bitboard from, Bitboard to) {
		assert(popcount(from) == 1);
		while (any(to))
			moves.push_back(Move(first_set(from), first_set(pop_lsb(to))));
	};

	// pawn non-capture moves
	if (!capture_only)
		for (auto bb = pawns; any(bb);)
		{
			auto from = pop_lsb(bb);
			auto to = pawn_moves(from, occupied, my_color == Color::White);
			gen(from, to);
		}

	// pawn (non-e.p.) capture moves
	for (auto bb = pawns; any(bb);)
	{
		auto from = pop_lsb(bb);
		auto to = pawn_attacks(from, my_color == Color::White);
		to &= them;
		gen(from, to);
	}

	// promotions
	{
		size_t tail = moves.size();
		for (size_t i = head; i < tail; ++i)
		{
			auto &m = moves[i];
			if (m.to >= 56 || m.to < 8)
			{
				m.promotion = PieceType::Queen;
				moves.push_back(m);
				m.promotion = PieceType::Rook;
				moves.push_back(m);
				m.promotion = PieceType::Bishop;
				moves.push_back(m);
				m.promotion = PieceType::Knight;
			}
		}
	}

	// en passant captures
	if (any(board.ep_square))
	{
		Bitboard to = board.ep_square;
		Bitboard from = my_color == Color::White ? down(to) : up(to);
		from = left(from) | right(from);
		from &= pawns;

		for (; any(from); pop_lsb(from))
			moves.push_back(Move(first_set(from), first_set(to)));
	}

	// non-pawn moves (capture and non-capture combined)
	for (auto bb = knights; any(bb);)
	{
		auto from = pop_lsb(bb);
		gen(from, knight_attacks(from) & mask);
	}
	for (auto bb = bishops | queens; any(bb);)
	{
		auto from = pop_lsb(bb);
		gen(from, bishop_attacks(from, me | them) & mask);
	}
	for (auto bb = rooks | queens; any(bb);)
	{
		auto from = pop_lsb(bb);
		gen(from, rook_attacks(from, me | them) & mask);
	}
	for (auto bb = kings; any(bb);)
	{
		auto from = pop_lsb(bb);
		gen(from, king_attacks(from) & mask);
	}

	// castling
	if (!capture_only)
	{
		int offset = my_color == Color::White ? 0 : 56;
		Bitboard attack_mask_kingside = Bitboard(uint64_t(112) << offset);
		Bitboard block_mask_kingside = Bitboard(uint64_t(96) << offset);
		Bitboard attack_mask_queenside = Bitboard(uint64_t(28) << offset);
		Bitboard block_mask_queenside = Bitboard(uint64_t(14) << offset);
		if (board.castling_right_kingside(my_color))
			if (none(occupied & block_mask_kingside))
				if (none(board.bb_attacks(-my_color) & attack_mask_kingside))
					moves.push_back(Move(4 + offset, 6 + offset));
		if (board.castling_right_queenside(my_color))
			if (none(occupied & block_mask_queenside))
				if (none(board.bb_attacks(-my_color) & attack_mask_queenside))
					moves.push_back(Move(4 + offset, 2 + offset));
	}
}
} // namespace metis