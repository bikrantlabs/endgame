#pragma once

#include "types.h"

constexpr int MATE_SCORE = 30000;

constexpr int PieceValue[PIECE_TYPE_NB] = {
    100, // Pawn
    320, // Knight
    330, // Bishop
    500, // Rook
    900, // Queen
    0    // King
};