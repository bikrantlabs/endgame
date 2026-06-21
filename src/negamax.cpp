#include "negamax.h"
#include "material.h"
#include "magic_bb.h"
#include "movegen.h"
#include "order_moves.h"
#include "search.h"
#include "tt.h"
#include "types.h"
#include "uci.h"
#include "utils.h"
#include "zobrist.h"
#include <algorithm>
#include <climits>
#include <iostream>

// Convert score to/from TT format with distance-to-mate adjustment
static int value_to_tt(int score, int ply) {
  if (abs(score) > MATE_SCORE - MAX_PLY)
    return score > 0 ? score + ply : score - ply;
  return score;
}

static int value_from_tt(int score, int ply) {
  if (abs(score) > MATE_SCORE - MAX_PLY)
    return score > 0 ? score - ply : score + ply;
  return score;
}

int negamax(Position &pos, int depth, int alpha, int beta, int ply,
            Move *out_best_move) {

  search_nodes++;

  if ((search_nodes & 2047) == 0) {
    if (time_is_up())
      search_stopped = true;
  }

  if (search_stopped)
    return 0;

  ZobrishKey key = pos.hash;
  TTEntry *tte = tt.probe(key);
  Move tt_move = -1;
  if (tte) {
    tt_move = tte->best_move;
    if (tte->depth >= depth && ply > 0) {
      int tt_score = value_from_tt(tte->score, ply);
      if (tte->flag == TT_EXACT) return tt_score;
      if (tte->flag == TT_ALPHA && tt_score <= alpha) return alpha;
      if (tte->flag == TT_BETA && tt_score >= beta) return beta;
    }
  }

  // Null Move Pruning
  if (depth >= 3 && !pos.is_in_check() &&
      pos.pieces[WHITE][QUEEN] != 0 && pos.pieces[BLACK][QUEEN] != 0) {
    ZobrishKey saved_hash = pos.hash;
    pos.hash ^= ZOBRIST.side;
    pos.side_to_move = static_cast<Color>(1 - pos.side_to_move);

    // Save full board state: the recursive call will modify pieces[][]
    Bitboard saved_pieces[COLOR_NB][PIECE_TYPE_NB];
    std::copy(&pos.pieces[0][0], &pos.pieces[0][0] + static_cast<int>(COLOR_NB) * static_cast<int>(PIECE_TYPE_NB),
              &saved_pieces[0][0]);
    Bitboard saved_occ[COLOR_NB] = {pos.occ[WHITE], pos.occ[BLACK]};
    Bitboard saved_all_occ = pos.all_occ;

    int reduction = 2 + depth / 4;
    int nd = depth - 1 - reduction;
    int score = -negamax(pos, nd, -beta, -beta + 1, ply);

    // Restore full board state
    std::copy(&saved_pieces[0][0], &saved_pieces[0][0] + static_cast<int>(COLOR_NB) * static_cast<int>(PIECE_TYPE_NB),
              &pos.pieces[0][0]);
    pos.occ[WHITE] = saved_occ[WHITE];
    pos.occ[BLACK] = saved_occ[BLACK];
    pos.all_occ = saved_all_occ;
    pos.side_to_move = static_cast<Color>(1 - pos.side_to_move);
    pos.hash = saved_hash;
    if (score >= beta)
      return beta;
  }

  if (depth == 0)
    return quiescence(pos, alpha, beta);

  MoveList move_list;
  gen_all_moves(pos, move_list);

  if (move_list.empty()) {
    if (pos.is_in_check())
      return -MATE_SCORE;
    return 0;
  }

  order_moves(pos, move_list, tt_move, ply);

  int best = -MATE_SCORE;
  Move best_move = 0;
  int orig_alpha = alpha;

  for (int i = 0; i < move_list.count; i++) {
    if (search_stopped)
      break;

    StateInfo st;
    Move m = move_list[i];
    pos.make_move(m, st);

    Color us = static_cast<Color>(1 - pos.side_to_move);
    bool king_captured = (pos.pieces[pos.side_to_move][KING] == 0);
    bool king_in_check = false;
    if (!king_captured)
        king_in_check = pos.is_square_attacked(Util::king_square(pos, us), pos.side_to_move);

    if (king_captured || king_in_check) {
      pos.unmake_move(st);
      continue;
    }
    int score = -negamax(pos, depth - 1, -beta, -alpha, ply + 1);

    pos.repair_consistency();
    pos.unmake_move(st);

    if (score > best) {
      best = score;
      best_move = move_list[i];
    }

    if (score > alpha)
      alpha = score;
    if (alpha >= beta) {
      if (!is_capture(m)) {
        store_killer(m, ply);
        update_history(m, depth);
      }
      break;
    }
  }

  TTFlag flag;
  if (best <= orig_alpha)
    flag = TT_ALPHA;
  else if (best >= beta)
    flag = TT_BETA;
  else
    flag = TT_EXACT;
  tt.store(key, depth, value_to_tt(best, ply), flag, best_move);

  if (out_best_move)
    *out_best_move = best_move;
  return best;
}
