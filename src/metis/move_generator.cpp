#include "metis/move_generator.h"

#include "metis/bitboard.h"
#include "metis/board.h"

namespace metis {

void generate_pseudolegal_moves(Board const &board, MoveList &moves)
{
	size_t head = moves.size();

	bool my_color = board.white_to_move;
	auto me = board.bb_color(my_color);
	auto them = board.bb_color(!my_color);
	auto occupied = me | them;

	auto pawns = board.bb_piece(PieceType::Pawn, my_color);
	auto knights = board.bb_piece(PieceType::Knight, my_color);
	auto bishops = board.bb_piece(PieceType::Bishop, my_color);
	auto rooks = board.bb_piece(PieceType::Rook, my_color);
	auto queens = board.bb_piece(PieceType::Queen, my_color);
	auto kings = board.bb_piece(PieceType::King, my_color);

	auto gen = [&](uint64_t from, uint64_t to) {
		assert(std::popcount(from) == 1);
		while (to)
		{
			moves.push_back(Move(std::countr_zero(from), std::countr_zero(to)));
			to &= to - 1;
		}
	};

	// pawn non-capture moves
	for (auto bb = pawns; bb;)
	{
		auto from = bb::pop_lsb(bb);
		auto to = my_color ? bb::white_pawn_moves(from, occupied)
		                   : bb::black_pawn_moves(from, occupied);
		gen(from, to);
	}

	// pawn (non-e.p.) capture moves
	for (auto bb = pawns; bb;)
	{
		auto from = bb::pop_lsb(bb);
		auto to = my_color ? bb::white_pawn_attacks(from)
		                   : bb::black_pawn_attacks(from);
		to &= them;
		gen(from, to);
	}

	// promotions
	{
		size_t tail = moves.size();
		for (size_t i = head; i < tail; ++i)
		{
			auto &m = moves[i];
			if ((my_color && m.to >= 56) || (!my_color && m.to < 8))
			{
				m.special = 3;
				m.promotion = 0;
				moves.push_back(m);
				m.promotion = 1;
				moves.push_back(m);
				m.promotion = 2;
				moves.push_back(m);
				m.promotion = 3;
			}
		}
	}

	// en passant captures
	if (board.ep_square != 0)
	{
		uint64_t to = board.ep_square;
		uint64_t from = my_color ? bb::down(to) : bb::up(to);
		from = bb::left(from) | bb::right(from);
		from &= pawns;

		for (; from; bb::pop_lsb(from))
			moves.push_back(
			    Move::ep(std::countr_zero(from), std::countr_zero(to)));
	}

	// non-pawn moves (capture and non-capture combined)
	for (auto bb = knights; bb;)
	{
		auto from = bb::pop_lsb(bb);
		gen(from, bb::knight_attacks(from) & ~me);
	}
	for (auto bb = bishops | queens; bb;)
	{
		auto from = bb::pop_lsb(bb);
		gen(from, bb::bishop_attacks(from, me | them) & ~me);
	}
	for (auto bb = rooks | queens; bb;)
	{
		auto from = bb::pop_lsb(bb);
		gen(from, bb::rook_attacks(from, me | them) & ~me);
	}
	for (auto bb = kings; bb;)
	{
		auto from = bb::pop_lsb(bb);
		gen(from, bb::king_attacks(from) & ~me);
	}

	// castling
	int offset = my_color ? 0 : 56;
	uint64_t attack_mask_kingside = uint64_t(112) << offset;
	uint64_t block_mask_kingside = uint64_t(96) << offset;
	uint64_t attack_mask_queenside = uint64_t(28) << offset;
	uint64_t block_mask_queenside = uint64_t(14) << offset;
	if (board.castling_right_kingside(my_color))
		if ((occupied & block_mask_kingside) == 0)
			if ((board.bb_attacks(!my_color) & attack_mask_kingside) == 0)
				moves.push_back(Move::castle(4 + offset, 6 + offset));
	if (board.castling_right_queenside(my_color))
		if ((occupied & block_mask_queenside) == 0)
			if ((board.bb_attacks(!my_color) & attack_mask_queenside) == 0)
				moves.push_back(Move::castle(4 + offset, 2 + offset));
}
} // namespace metis