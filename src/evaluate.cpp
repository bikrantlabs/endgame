#include "evaluate.h"
#include "material.h"
#include "psqt.h"
#include "types.h"

int evaluate(const Position &pos) {
  int score = 0;
  score += evaluate_material(pos);
  score += evaluate_pst(pos);
  return score;
}

int evaluate_material(const Position &pos) {
  int score = 0;

  for (int pt = PAWN; pt < KING; ++pt) {
    score += popcount(pos.pieces[WHITE][pt]) * PieceValue[pt];
    score -= popcount(pos.pieces[BLACK][pt]) * PieceValue[pt];
  }

  //   Return from side-to-move's perspective
  return pos.side_to_move == WHITE ? score : -score;
}

int evaluate_pst(const Position &pos) {
  int score = 0;
  for (int pt = PAWN; pt < KING; ++pt) {
    Bitboard wb = pos.pieces[WHITE][pt];

    while (wb) {
      int sq = unset_lsb(wb);
      score += pst_value(WHITE, static_cast<PieceType>(pt), sq);
    }

    Bitboard bb = pos.pieces[BLACK][pt];

    while (bb) {
      int sq = unset_lsb(bb);
      score -= pst_value(BLACK, static_cast<PieceType>(pt), sq);
    }
  }
  return pos.side_to_move == WHITE ? score : -score;
}