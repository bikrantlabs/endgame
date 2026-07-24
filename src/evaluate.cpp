#include "evaluate.h"
#include "material.h"
#include "psqt.h"
#include "types.h"

int evaluate(const Position &pos) {
  int phase = game_phase(pos);
  int score = 0;
  score += evaluate_material(pos, phase);
  score += evaluate_pst(pos, phase);
  return score;
}

int game_phase(const Position &pos) {
  int phase = 0;
  for (int pt = KNIGHT; pt <= QUEEN; ++pt) {
    phase += popcount(pos.pieces[WHITE][pt]) * PhaseWeight[pt];
    phase += popcount(pos.pieces[BLACK][pt]) * PhaseWeight[pt];
  }
  if (phase > MAX_PHASE)
    phase = MAX_PHASE;
  return phase;
}

int evaluate_material(const Position &pos, int phase) {
  int mg = 0, eg = 0;

  for (int pt = PAWN; pt <= KING; ++pt) {
    int count_w = popcount(pos.pieces[WHITE][pt]);
    int count_b = popcount(pos.pieces[BLACK][pt]);
    mg += count_w * PieceValueMG[pt];
    mg -= count_b * PieceValueMG[pt];
    eg += count_w * PieceValueEG[pt];
    eg -= count_b * PieceValueEG[pt];
  }

  int score = (mg * phase + eg * (MAX_PHASE - phase)) / MAX_PHASE;
  return pos.side_to_move == WHITE ? score : -score;
}

int evaluate_pst(const Position &pos, int phase) {
  int mg = 0, eg = 0;

  for (int pt = PAWN; pt <= KING; ++pt) {
    Bitboard wb = pos.pieces[WHITE][pt];
    while (wb) {
      int sq = unset_lsb(wb);
      mg += pst_value_mg(WHITE, static_cast<PieceType>(pt), sq);
      eg += pst_value_eg(WHITE, static_cast<PieceType>(pt), sq);
    }

    Bitboard bb = pos.pieces[BLACK][pt];
    while (bb) {
      int sq = unset_lsb(bb);
      mg -= pst_value_mg(BLACK, static_cast<PieceType>(pt), sq);
      eg -= pst_value_eg(BLACK, static_cast<PieceType>(pt), sq);
    }
  }

  int score = (mg * phase + eg * (MAX_PHASE - phase)) / MAX_PHASE;
  return pos.side_to_move == WHITE ? score : -score;
}
