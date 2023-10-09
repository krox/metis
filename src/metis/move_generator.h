#pragma once

#include "metis/board.h"
#include "util/vector.h"

namespace metis {

// Maximum number of legal moves from any position.
// Best lower limit seems to be 218 (in a very contrived,
// unrealistic-to-ever-happen position of course), so 255 should be plenty.
static constexpr int MAX_LEGAL_MOVES = 255;

// Maximum number of plies in a game. Theory says 5949 (50 moves in between
// pawn-pushed/captures), but in practice this is plenty.
static constexpr int MAX_PLIES = 512;

// a list of moves, typically a result of move generation
using MoveList = util::static_vector<Move, MAX_LEGAL_MOVES>;

// generate all legal moves plus some pseudolegal moves
// (can be filtered out by calling .is_legal() on the resulting positions)
// TODO: some flags for restricting moves (caputures only, tactical only, etc.)
void generate_pseudolegal_moves(Board const &board, MoveList &moves);

} // namespace metis