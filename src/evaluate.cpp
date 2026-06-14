#include "evaluate.h"
#include "material.h"

int evaluate(const Position &pos) {
  int score = 0;
  score += evaluate_material(pos);

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