#include "movegen.h"
#include "types.h"
#include <cassert>

// The knight attacks and king attacks are precomputed.
Bitboard KNIGHT_ATTACKS[64];
Bitboard KING_ATTACKS[64];

static void init_knight_attacks() {
  for (int square = 0; square < 64; ++square) {
    int rank = sq_rank(square), file = sq_file(square);

    Bitboard attacks = 0ULL;

    /// The rank offsets and file offsets works in pair to determine the legal
    /// move for knight in any square
    /// Knight always moves in L shape (2 square + 1 square diagonal)
    /// rank_offsets[0] and file_offsets[0] means: 2 rank up and 1 file left
    /// (producing L shape)
    const int rank_offsets[] = {-2, -2, -1, -1, 1, 1, 2, 2};
    const int file_offsets[] = {-1, 1, -2, 2, -2, 2, -1, 1};

    for (int i = 0; i < 8; ++i) {
      int target_rank = rank + rank_offsets[i];
      int target_file = file + file_offsets[i];

      /// Bound between 0 and 7(inclusive) to prevent generating outside-board
      /// moves
      if (target_rank >= 0 && target_rank < 8 && target_file >= 0 &&
          target_file < 8)
        /// OR operation of previous attack with target file/rank square to
        /// build new attacks for specific square.
        attacks = attacks | sq_bb(sq_of(target_file, target_rank));
    }
    KNIGHT_ATTACKS[square] = attacks;
  }
}

/// Generate 8 squares around a king for every square
static void init_king_attacks() {
  for (int square = 0; square < 64; ++square) {
    int rank = sq_rank(square), file = sq_file(square);

    Bitboard attacks = 0ULL;

    for (int rank_delta = -1; rank_delta <= 1; ++rank_delta)
      for (int rank_file = -1; rank_file <= 1; ++rank_file) {
        if (rank_delta == 0 && rank_file == 0) {
          // Center Position
          continue;
        }
        int target_rank = rank + rank_delta, target_file = file + rank_file;
        /// Bound between 0 and 7(inclusive) to prevent generating outside-board
        /// moves
        if (target_rank >= 0 && target_rank < 8 && target_file >= 0 &&
            target_file < 8)
          attacks = attacks | sq_bb(sq_of(target_file, target_rank));
      }
    KING_ATTACKS[square] = attacks;
  }
}

void init_movegen() {
  init_knight_attacks();
  init_king_attacks();
}

//  Pawn quiet move generation
void gen_pawn_quiet(const Position &pos, MoveList &ml) {
  Color us = pos.side_to_move;

  Bitboard pawns = pos.pieces[us][PAWN];
  Bitboard enemy_occ = pos.occ[1 - us]; // occupancy board for opponent.
  Bitboard empty = ~pos.all_occ;

  // Everything that differs between white and black
  const Bitboard start_rank = (us == WHITE) ? RANK_2 : RANK_7;
  const Bitboard promo_rank = (us == WHITE) ? RANK_8 : RANK_1;
  const int capture_dir_right = (us == WHITE) ? 9 : -7; // Diagonal Left
  const int capture_dir_left = (us == WHITE) ? 7 : -9;  // Diagonal Left
  const int push_dir = (us == WHITE) ? 8 : -8;

  auto shift_forward = [&](Bitboard b) {
    return (us == WHITE) ? shift_north(b) : shift_south(b);
  };

  // --- Single push ---
  Bitboard single = shift_forward(pawns) & empty;

  while (single) {
    int to_sq = unset_lsb(single);
    int from_sq = to_sq - push_dir;
    // promotion check goes here later
    ml.add(make_move(from_sq, to_sq, QUIET));
  }

  // --- Double push ---
  Bitboard on_start = pawns & start_rank;
  if (on_start) {
    Bitboard one_step = shift_forward(on_start) & empty;
    Bitboard two_step = shift_forward(one_step) & empty;
    while (two_step) {
      int to_sq = unset_lsb(two_step);
      int from_sq = to_sq - push_dir * 2;
      ml.add(make_move(from_sq, to_sq, DOUBLE_PUSH));
    }
  }

  // Captures- If there is any (opponent) piece in diagonals, mark that move as
  // capture.
  // -- Right Capture
  single = (us == WHITE ? shift_ne(pawns) : shift_se(pawns)) & enemy_occ;
  while (single) {
    int to_sq = unset_lsb(single);
    int from_sq = to_sq - capture_dir_right;
    ml.add(make_move(from_sq, to_sq, CAPTURE));
  }
  // -- Left Capture
  single = (us == WHITE ? shift_nw(pawns) : shift_sw(pawns)) & enemy_occ;
  while (single) {
    int to_sq = unset_lsb(single);
    int from_sq = to_sq - capture_dir_left;
    ml.add(make_move(from_sq, to_sq, CAPTURE));
  }

  // --- Captures, en passant, promotions go here later ---
  // same pattern: shift_ne/nw for white, shift_se/sw for black
  // just add capture_dir_left / capture_dir_right constants above
}

void gen_all_quiet(const Position &pos, MoveList &ml) {
  gen_pawn_quiet(pos, ml);
}

/// Generate moves only for the piece on a specific square
/// Only use this method for UI purposes, not for AI.
bool gen_moves_for_square(const Position &pos, int sq, MoveList &ml) {
  Color us = pos.side_to_move;
  if (pos.color_on(sq) != us)
    return false; // no friendly piece there

  PieceType pt = pos.piece_on(us, sq);
  if (pt == PIECE_TYPE_NB)
    return false;

  // Generate all moves for this side, then filter to those from sq
  MoveList all;
  gen_all_quiet(pos, all);

  for (int i = 0; i < all.count; ++i) {
    if (move_from(all.moves[i]) == sq)
      ml.add(all.moves[i]);
  }
  return true;
}
