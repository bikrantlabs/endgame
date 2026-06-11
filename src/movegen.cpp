#include "movegen.h"
#include "magic_bb.h"
#include "types.h"
#include "utils.h"
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
  init_magic_bitboards();
}

//  Pawn quiet move generation
void gen_pawn_moves(const Position &pos, MoveList &ml) {
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

  // Enpassant square is set whenever opponent double push, just check if my
  // pawn can reach that file diagonally left or right.
  // Enpassant square is automatically set in position.cpp->make_move() function
  // whenever it detects double_push move.
  if (pos.ep_square != NO_SQUARE) {
    Bitboard ep_bb = sq_bb(pos.ep_square);

    Bitboard can_capture_right =
        (us == WHITE) ? shift_ne(pawns) & ep_bb : shift_se(pawns) & ep_bb;
    Bitboard can_capture_left =
        (us == WHITE) ? shift_nw(pawns) & ep_bb : shift_sw(pawns) & ep_bb;

    if (can_capture_right) {
      int from_sq = pos.ep_square - capture_dir_right;
      ml.add(make_move(from_sq, pos.ep_square, EP_CAPTURE));
    }

    if (can_capture_left) {
      int from_sq = pos.ep_square - capture_dir_left;
      ml.add(make_move(from_sq, pos.ep_square, EP_CAPTURE));
    }
  }
  // same pattern: shift_ne/nw for white, shift_se/sw for black
  // just add capture_dir_left / capture_dir_right constants above
}

void gen_knight_moves(const Position &pos, MoveList &ml) {
  Color us = pos.side_to_move;

  Bitboard knights = pos.pieces[us][KNIGHT];
  Bitboard enemy_occ = pos.occ[1 - us]; // occupancy board for opponent.

  while (knights) {
    int from = unset_lsb(knights);

    // Filter out attacks squares where there is friendly pieces.
    Bitboard attacks = KNIGHT_ATTACKS[from] & ~pos.occ[us];

    while (attacks) {
      int to = unset_lsb(attacks);

      // If target(to) square contains any enemy piece
      if (test_bit(enemy_occ, to)) {
        ml.add(make_move(from, to, CAPTURE));
      } else {
        ml.add(make_move(from, to, QUIET));
      }
    }
  }
}
void gen_king_moves(const Position &pos, MoveList &ml) {
  Color us = pos.side_to_move;
  Color them = static_cast<Color>(1 - us);

  Bitboard king = pos.pieces[us][KING];
  Bitboard enemy_occ = pos.occ[them];

  while (king) {
    int from = unset_lsb(king);

    // Normal king moves
    Bitboard attacks = KING_ATTACKS[from] & ~pos.occ[us];

    while (attacks) {
      int to = unset_lsb(attacks);

      // King may not move into check
      if (pos.is_square_attacked(to, them))
        continue;

      if (test_bit(enemy_occ, to))
        ml.add(make_move(from, to, CAPTURE));
      else
        ml.add(make_move(from, to, QUIET));
    }

    // Castling

    int king_side_right = (us == WHITE) ? WK_CASTLE : BK_CASTLE;

    int queen_side_right = (us == WHITE) ? WQ_CASTLE : BQ_CASTLE;

    int king_dest = (us == WHITE) ? G1 : G8;

    int queen_dest = (us == WHITE) ? C1 : C8;

    int king_path1 = (us == WHITE) ? F1 : F8;

    int king_path2 = (us == WHITE) ? G1 : G8;

    int queen_path1 = (us == WHITE) ? D1 : D8;

    int queen_path2 = (us == WHITE) ? C1 : C8;

    int queen_path3 = (us == WHITE) ? B1 : B8;

    // Kingside castle
    if (pos.castling_rights & king_side_right) {

      if (!pos.is_occupied(king_path1) && !pos.is_occupied(king_path2) &&
          !pos.is_square_attacked(from, them) &&
          !pos.is_square_attacked(king_path1, them) &&
          !pos.is_square_attacked(king_path2, them)) {
        ml.add(make_move(from, king_dest, KING_CASTLE));
      }
    }

    // Queenside castle
    if (pos.castling_rights & queen_side_right) {

      if (!pos.is_occupied(queen_path1) && !pos.is_occupied(queen_path2) &&
          !pos.is_occupied(queen_path3) &&
          !pos.is_square_attacked(from, them) &&
          !pos.is_square_attacked(queen_path1, them) &&
          !pos.is_square_attacked(queen_path2, them)) {
        ml.add(make_move(from, queen_dest, QUEEN_CASTLE));
      }
    }
  }
}
void gen_rook_moves(const Position &pos, MoveList &ml) {
  Color us = pos.side_to_move;
  Bitboard rooks = pos.pieces[us][ROOK];
  Bitboard enemy = pos.occ[1 - us];

  while (rooks) {
    int from = unset_lsb(rooks);
    Bitboard attacks = rook_attacks(from, pos.all_occ) & ~pos.occ[us];
    while (attacks) {
      int to = unset_lsb(attacks);
      ml.add(make_move(from, to, test_bit(enemy, to) ? CAPTURE : QUIET));
    }
  }
}
void gen_bishop_moves(const Position &pos, MoveList &ml) {
  Color us = pos.side_to_move;
  Bitboard bishop = pos.pieces[us][BISHOP];
  Bitboard enemy = pos.occ[1 - us];

  while (bishop) {
    int from = unset_lsb(bishop);
    Bitboard attacks = bishop_attacks(from, pos.all_occ) & ~pos.occ[us];
    while (attacks) {
      int to = unset_lsb(attacks);
      ml.add(make_move(from, to, test_bit(enemy, to) ? CAPTURE : QUIET));
    }
  }
}
void gen_queen_moves(const Position &pos, MoveList &ml) {
  Color us = pos.side_to_move;
  Bitboard queen = pos.pieces[us][QUEEN];
  Bitboard enemy = pos.occ[1 - us];

  while (queen) {
    int from = unset_lsb(queen);
    Bitboard attacks = queen_attacks(from, pos.all_occ) & ~pos.occ[us];
    while (attacks) {
      int to = unset_lsb(attacks);
      ml.add(make_move(from, to, test_bit(enemy, to) ? CAPTURE : QUIET));
    }
  }
}

void gen_all_moves(const Position &pos, MoveList &ml) {
  gen_pawn_moves(pos, ml);
  gen_knight_moves(pos, ml);
  gen_king_moves(pos, ml);
  gen_rook_moves(pos, ml);
  gen_queen_moves(pos, ml);
  gen_bishop_moves(pos, ml);
}
void gen_legal_moves(Position &pos, MoveList &legal) {

  MoveList pseudo;
  gen_all_moves(pos, pseudo);

  legal.clear();

  for (int i = 0; i < pseudo.count; i++) {

    Move m = pseudo[i];

    StateInfo st;
    pos.make_move(m, st);

    // After make_move, side_to_move is the opponent.
    // Check if the side that just moved left their king in check.
    Color us = static_cast<Color>(1 - pos.side_to_move);
    Color them = pos.side_to_move;
    bool in_check =
        pos.is_square_attacked(Util::king_square(pos, us), them);

    if (!in_check) {
      legal.add(m);
    }

    pos.unmake_move(st);
  }
}

/// Generate moves only for the piece on a specific square
/// Only use this method for UI purposes, not for AI.
bool gen_moves_for_square(Position &pos, int sq, MoveList &ml) {
  Color us = pos.side_to_move;
  if (pos.color_on(sq) != us)
    return false; // no friendly piece there

  PieceType pt = pos.piece_on(us, sq);
  if (pt == PIECE_TYPE_NB)
    return false;

  // Generate all moves for this side, then filter to those from sq
  MoveList all;
  gen_legal_moves(pos, all);

  for (int i = 0; i < all.count; ++i) {
    if (move_from(all.moves[i]) == sq)
      ml.add(all.moves[i]);
  }
  return true;
}
