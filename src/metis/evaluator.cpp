#include "metis/evaluator.h"

#include "Eigen/Dense"
#include "metis/board.h"
#include "util/json.h"
#include <bit>

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

void LinearEvaluator::set_weights(Eigen::VectorXf const &weights)
{
	assert(weights.size() == (int)terms_.size());
	for (size_t i = 0; i < terms_.size(); ++i)
		terms_[i].score = int(1e4 * weights[i]);
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

void LinearEvaluator::get_features(Board const &board,
                                   Eigen::VectorXf &features) const
{
	assert(features.size() == (int)terms_.size());

	for (size_t i = 0; i < terms_.size(); ++i)
	{
		auto term = terms_[i];
		auto bb_white = board.bb_piece(term.pt, Color::White);
		auto bb_black = board.bb_piece(term.pt, Color::Black);
		features[i] =
		    popcount(term.bb & bb_white) - popcount(term.bb & bb_black);
	}
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