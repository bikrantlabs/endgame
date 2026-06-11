#pragma once
#include "position.h"
#include "types.h"

// ─────────────────────────────────────────────
//  Move Generation
//
//  Phase 1: quiet moves only (no captures).
//  Each gen_* function appends to the provided MoveList.
// ─────────────────────────────────────────────

// Call once at program startup to fill lookup tables
void init_movegen();

// Pawn quiet moves: single push + double push (no captures, no promotions yet)
void gen_pawn_moves(const Position &pos, MoveList &ml);

// Knight quiet moves
void gen_knight_moves(const Position &pos, MoveList &ml);

// Bishop quiet moves (ray casting)
void gen_bishop_moves(const Position &pos, MoveList &ml);

// Rook  moves (ray casting)
void gen_rook_moves(const Position &pos, MoveList &ml);

// Queen  moves (bishop + rook combined)
void gen_queen_moves(const Position &pos, MoveList &ml);

// King  moves (no castling yet)
void gen_king_moves(const Position &pos, MoveList &ml);

// Generate ALL  moves for the side to move
void gen_all_moves(const Position &pos, MoveList &ml);

// Generate moves only for the piece on sq (for the selected-piece UI)
// Returns false if there's no friendly piece on sq.
bool gen_moves_for_square(Position &pos, int sq, MoveList &ml);

// Generate only legal moves (filters out moves that leave king in check)
void gen_legal_moves(Position &pos, MoveList &ml);

// ─────────────────────────────────────────────
//  Attack tables (precomputed at startup)
// ─────────────────────────────────────────────

extern Bitboard KNIGHT_ATTACKS[64];
extern Bitboard KING_ATTACKS[64];

// Ray attack helpers (classical sliding piece generation)
Bitboard rook_attacks_otf(int sq, Bitboard occ);
Bitboard bishop_attacks_otf(int sq, Bitboard occ);
