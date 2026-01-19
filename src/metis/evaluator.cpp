#include "metis/evaluator.h"

#include "metis/board.h"
#include "util/json.h"
#include <algorithm>
#include <bit>
#include <cstdlib>

namespace metis {

namespace {
PieceType str_to_piecetype(std::string_view s)
{
	if (s == "pawn")
		return PieceType::Pawn;
	if (s == "knight")
		return PieceType::Knight;
	if (s == "bishop")
		return PieceType::Bishop;
	if (s == "rook")
		return PieceType::Rook;
	if (s == "queen")
		return PieceType::Queen;
	if (s == "king")
		return PieceType::King;
	throw std::runtime_error("invalid piece type");
}

std::string to_string(PieceType pt)
{
	switch (pt)
	{
	case PieceType::Pawn:
		return "pawn";
	case PieceType::Knight:
		return "knight";
	case PieceType::Bishop:
		return "bishop";
	case PieceType::Rook:
		return "rook";
	case PieceType::Queen:
		return "queen";
	case PieceType::King:
		return "king";
	default:
		throw std::runtime_error("invalid piece type");
	}
}
} // namespace

int MaterialEvaluator::evaluate(Board const &board) const
{
	// part 1: material value
	int w_pawns = popcount(board.bb_piece(PieceType::Pawn, Color::White));
	int b_pawns = popcount(board.bb_piece(PieceType::Pawn, Color::Black));
	int w_knights = popcount(board.bb_piece(PieceType::Knight, Color::White));
	int b_knights = popcount(board.bb_piece(PieceType::Knight, Color::Black));
	int w_bishops = popcount(board.bb_piece(PieceType::Bishop, Color::White));
	int b_bishops = popcount(board.bb_piece(PieceType::Bishop, Color::Black));
	int w_rooks = popcount(board.bb_piece(PieceType::Rook, Color::White));
	int b_rooks = popcount(board.bb_piece(PieceType::Rook, Color::Black));
	int w_queens = popcount(board.bb_piece(PieceType::Queen, Color::White));
	int b_queens = popcount(board.bb_piece(PieceType::Queen, Color::Black));

	int score = 0;
	score += 1000 * (w_pawns - b_pawns);
	score += 3100 * (w_knights - b_knights);
	score += 3300 * (w_bishops - b_bishops);
	score += 5000 * (w_rooks - b_rooks);
	score += 9000 * (w_queens - b_queens);

	// part 2: slight bonus for board control (intended as tie-breaker in the
	// opening)
	score += 5 * (popcount(board.bb_attacks(Color::White)) -
	              popcount(board.bb_attacks(Color::Black)));
	return score;
}

LinearEvaluator::LinearEvaluator(util::Json const &json)
{
	for (auto [key, value] : json.at("terms").as_object())
	{
		auto parts = util::split(key, '-');
		assert(parts.size() == 2);
		PieceType pt = str_to_piecetype(parts[0]);
		Bitboard bb = bitboard(parts[1]);
		int score = value.get<int>();
		add_term(Term{.bb = bb, .pt = pt, .score = score});
	}
}

int LinearEvaluator::evaluate(Board const &board) const
{
	int score = 0;
	for (auto term : terms_)
	{
		auto bb_white = board.bb_piece(term.pt, Color::White);
		auto bb_black = board.bb_piece(term.pt, Color::Black);
		score += term.score *
		         (popcount(term.bb & bb_white) - popcount(term.bb & bb_black));
	}
	return score;
}

util::Json to_json(LinearEvaluator const &eval)
{
	auto terms = util::Json::object();
	for (auto term : eval.terms())
	{
		auto name =
		    fmt::format("{}-{}", to_string(term.pt), to_string(term.bb));
		terms[name] = term.score;
	}

	util::Json json;
	json["terms"] = std::move(terms);
	return json;
}
} // namespace metis