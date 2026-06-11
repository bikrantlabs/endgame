#include "magic_bb.h"
#include "types.h"

// Storage for the extern arrays declared in magic.h
Bitboard ROOK_ATTACKS[64][ROOK_TABLE_SIZE];
Bitboard BISHOP_ATTACKS[64][BISHOP_TABLE_SIZE];
Bitboard ROOK_MASKS[64];
Bitboard BISHOP_MASKS[64];

//  STEP 1 — Mask generation
//
//  The mask for a square is the set of squares that CAN block the piece.
//  Board edges are excluded: a piece on the edge can't block "further" movement
//  so they add no information and only waste table space.
//
//  We build the mask by walking each ray and stopping one square before the
//  board edge (i.e., we stop before rank 1, rank 8, file A, file H).

Bitboard compute_rook_mask(int sq) {
  Bitboard mask = 0ULL;
  int r = sq_rank(sq), f = sq_file(sq);

  // North ray — stop before rank 7 (index 7 = rank 8)
  for (int rr = r + 1; rr <= 6; ++rr)
    mask |= sq_bb(sq_of(f, rr));
  // South ray — stop before rank 0 (rank 1)
  for (int rr = r - 1; rr >= 1; --rr)
    mask |= sq_bb(sq_of(f, rr));
  // East ray — stop before file 7 (file H)
  for (int ff = f + 1; ff <= 6; ++ff)
    mask |= sq_bb(sq_of(ff, r));
  // West ray — stop before file 0 (file A)
  for (int ff = f - 1; ff >= 1; --ff)
    mask |= sq_bb(sq_of(ff, r));

  return mask;
}

Bitboard compute_bishop_mask(int sq) {
  Bitboard mask = 0ULL;
  int r = sq_rank(sq), f = sq_file(sq);

  // NE diagonal
  for (int rr = r + 1, ff = f + 1; rr <= 6 && ff <= 6; ++rr, ++ff)
    mask |= sq_bb(sq_of(ff, rr));
  // NW diagonal
  for (int rr = r + 1, ff = f - 1; rr <= 6 && ff >= 1; ++rr, --ff)
    mask |= sq_bb(sq_of(ff, rr));
  // SE diagonal
  for (int rr = r - 1, ff = f + 1; rr >= 1 && ff <= 6; --rr, ++ff)
    mask |= sq_bb(sq_of(ff, rr));
  // SW diagonal
  for (int rr = r - 1, ff = f - 1; rr >= 1 && ff >= 1; --rr, --ff)
    mask |= sq_bb(sq_of(ff, rr));

  return mask;
}

//  STEP 2 — Actual attack computation (ray-cast, used ONLY during init)
//
//  Given a blocker occupancy, walk each ray and stop AT the first blocker
//  (include the blocker square — it's a capture target).
//  This is the "slow" version used once during startup to fill the table.

Bitboard compute_rook_attacks_slow(int sq, Bitboard blockers) {
  Bitboard attacks = 0ULL;
  int r = sq_rank(sq), f = sq_file(sq);

  for (int rr = r + 1; rr <= 7; ++rr) {
    attacks |= sq_bb(sq_of(f, rr));
    if (blockers & sq_bb(sq_of(f, rr)))
      break; // blocked — stop after this sq
  }
  for (int rr = r - 1; rr >= 0; --rr) {
    attacks |= sq_bb(sq_of(f, rr));
    if (blockers & sq_bb(sq_of(f, rr)))
      break;
  }
  for (int ff = f + 1; ff <= 7; ++ff) {
    attacks |= sq_bb(sq_of(ff, r));
    if (blockers & sq_bb(sq_of(ff, r)))
      break;
  }
  for (int ff = f - 1; ff >= 0; --ff) {
    attacks |= sq_bb(sq_of(ff, r));
    if (blockers & sq_bb(sq_of(ff, r)))
      break;
  }

  return attacks;
}

Bitboard compute_bishop_attacks_slow(int sq, Bitboard blockers) {
  Bitboard attacks = 0ULL;
  int r = sq_rank(sq), f = sq_file(sq);

  for (int rr = r + 1, ff = f + 1; rr <= 7 && ff <= 7; ++rr, ++ff) {
    attacks |= sq_bb(sq_of(ff, rr));
    if (blockers & sq_bb(sq_of(ff, rr)))
      break;
  }
  for (int rr = r + 1, ff = f - 1; rr <= 7 && ff >= 0; ++rr, --ff) {
    attacks |= sq_bb(sq_of(ff, rr));
    if (blockers & sq_bb(sq_of(ff, rr)))
      break;
  }
  for (int rr = r - 1, ff = f + 1; rr >= 0 && ff <= 7; --rr, ++ff) {
    attacks |= sq_bb(sq_of(ff, rr));
    if (blockers & sq_bb(sq_of(ff, rr)))
      break;
  }
  for (int rr = r - 1, ff = f - 1; rr >= 0 && ff >= 0; --rr, --ff) {
    attacks |= sq_bb(sq_of(ff, rr));
    if (blockers & sq_bb(sq_of(ff, rr)))
      break;
  }

  return attacks;
}

//  Enumerate all subsets of a mask  ("Carry-Rippler" trick)
//
//  Given a mask with N bits, there are 2^N subsets.
//  This loop visits every one of them:
//
//      Bitboard subset = 0;
//      do {
//          // use subset ...
//          subset = (subset - mask) & mask;
//      } while (subset != 0);
//

void init_magic_bitboards() {
  for (int sq = 0; sq < 64; ++sq) {

    // --- Rooks ---
    ROOK_MASKS[sq] = compute_rook_mask(sq);
    Bitboard mask = ROOK_MASKS[sq];

    // Enumerate every subset of this mask
    Bitboard subset = 0ULL;
    do {
      // Hash the subset to an index using the magic number
      int index = (int)((subset * ROOK_MAGICS[sq]) >> ROOK_SHIFTS[sq]);
      // Store the actual attacks for this blocker pattern
      ROOK_ATTACKS[sq][index] = compute_rook_attacks_slow(sq, subset);
      subset = (subset - mask) & mask;
    } while (subset != 0ULL);

    // --- Bishops ---
    BISHOP_MASKS[sq] = compute_bishop_mask(sq);
    mask = BISHOP_MASKS[sq];

    subset = 0ULL;
    do {
      int index = (int)((subset * BISHOP_MAGICS[sq]) >> BISHOP_SHIFTS[sq]);
      BISHOP_ATTACKS[sq][index] = compute_bishop_attacks_slow(sq, subset);
      subset = (subset - mask) & mask;
    } while (subset != 0ULL);
  }
}

Bitboard debug_bishop_slow(int sq, Bitboard blockers) {
  return compute_bishop_attacks_slow(sq, blockers);
}