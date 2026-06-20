#include "negamax.h"
#include "evaluate.h"
#include "material.h"
#include "movegen.h"
#include "order_moves.h"
#include "tt.h"
#include "types.h"
#include "uci.h"
#include <climits>

int negamax(Position &pos, int depth, int alpha, int beta, int ply,
            Move *out_best_move) {

  search_nodes++;
  ZobrishKey key = pos.hash;
  TTEntry *tte = tt.probe(key);
  Move tt_move = -1;
  if (tte) {
    tt_move = tte->best_move;
    if (tte->depth >= depth) {
      if (tte->flag == TT_EXACT)
        return tte->score;
      if (tte->flag == TT_ALPHA && tte->score <= alpha)
        return alpha;
      if (tte->flag == TT_BETA && tte->score >= beta)
        return beta;
    }
  }

  if (search_stopped)
    return 0;

  if (depth == 0)
    return quiescence(pos, alpha, beta);

  MoveList move_list;
  gen_legal_moves(pos, move_list);

  if (move_list.empty()) {
    if (pos.is_in_check())
      return -MATE_SCORE;
    return 0;
  }

  order_moves(pos, move_list, tt_move, ply); // <-- NEW

  int best = INT_MIN;
  Move best_move = 0;
  int orig_alpha = alpha;

  for (int i = 0; i < move_list.count; i++) {
    if (search_stopped)
      break;

    StateInfo st;
    Move m = move_list[i];
    pos.make_move(m, st);

    int score = -negamax(pos, depth - 1, -beta, -alpha, ply + 1);

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
  tt.store(key, depth, best, flag, best_move);

  if (out_best_move)
    *out_best_move = best_move;
  return best;
}
