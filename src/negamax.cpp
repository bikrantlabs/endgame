#include "negamax.h"
#include "evaluate.h"
#include "movegen.h"
#include "types.h"
#include "uci.h"
#include <climits>

int negamax(Position &pos, int depth, int alpha, int beta) {

  search_nodes++;

  if (search_stopped)
    return 0;

  if (depth == 0)
    return evaluate(pos);

  MoveList move_list;
  gen_legal_moves(pos, move_list);

  int best = INT_MIN;

  for (int i = 0; i < move_list.count; i++) {
    if (search_stopped)
      break;

    StateInfo st;
    Move m = move_list[i];
    pos.make_move(m, st);

    int score = -negamax(pos, depth - 1, -beta, -alpha);

    pos.unmake_move(st);

    if (score > best)
      best = score;
    if (score > alpha)
      alpha = score;
    if (alpha >= beta)
      break;
  }
  return best;
}
