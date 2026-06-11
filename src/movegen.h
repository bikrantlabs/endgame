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
void gen_bishop_quiet(const Position &pos, MoveList &ml);

// Rook quiet moves (ray casting)
void gen_rook_quiet(const Position &pos, MoveList &ml);

// Queen quiet moves (bishop + rook combined)
void gen_queen_quiet(const Position &pos, MoveList &ml);

// King quiet moves (no castling yet)
void gen_king_quiet(const Position &pos, MoveList &ml);

// Generate ALL quiet moves for the side to move
void gen_all_quiet(const Position &pos, MoveList &ml);

// Generate quiet moves only for the piece on sq (for the selected-piece UI)
// Returns false if there's no friendly piece on sq.
bool gen_moves_for_square(const Position &pos, int sq, MoveList &ml);

// ─────────────────────────────────────────────
//  Attack tables (precomputed at startup)
// ─────────────────────────────────────────────

extern Bitboard KNIGHT_ATTACKS[64];
extern Bitboard KING_ATTACKS[64];

// Ray attack helpers (classical sliding piece generation)
Bitboard rook_attacks_otf(int sq, Bitboard occ);
Bitboard bishop_attacks_otf(int sq, Bitboard occ);
