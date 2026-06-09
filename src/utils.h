#pragma once

#include "types.h"

// Declare the namespace
namespace Util {

/// Get the piece name from piece type
const char *piece_name(PieceType pt);

/// Find single move within the legal move list
Move find_move(const MoveList &legal, int from, int to);

/// Print legal targets(destinations moves) for the piece
void print_targets(const MoveList &ml);
} // namespace Util
