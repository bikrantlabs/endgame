#pragma once

#include "types.h"

constexpr int MATE_SCORE = 30000;

// Tapered evaluation: max phase = 24 (4 knights + 4 bishops + 4 rooks + 2 queens)
constexpr int MAX_PHASE = 24;

// PeSTO-tuned middlegame piece values
constexpr int PieceValueMG[PIECE_TYPE_NB] = {
    82,   // PAWN
    337,  // KNIGHT
    365,  // BISHOP
    477,  // ROOK
    1025, // QUEEN
    0     // KING
};

// PeSTO-tuned endgame piece values
constexpr int PieceValueEG[PIECE_TYPE_NB] = {
    94,   // PAWN
    281,  // KNIGHT
    297,  // BISHOP
    512,  // ROOK
    936,  // QUEEN
    0     // KING
};

// Phase increment per piece type (pawns and kings don't contribute)
constexpr int PhaseWeight[PIECE_TYPE_NB] = {0, 1, 1, 2, 4, 0};
