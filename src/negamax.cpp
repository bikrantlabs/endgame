#include "negamax.h"
#include "evaluate.h"
#include "movegen.h"
#include "types.h"
#include <climits>
int negamax(Position &pos, int depth) {

  if (depth == 0)
    return evaluate(pos);

  // TODO: Check isTerminal(pos) -> No moves: checkmate or stalemate
  MoveList move_list;

  gen_legal_moves(pos, move_list);

  int best = INT_MIN;

  for (int i = 0; i < move_list.count; i++) {
    StateInfo st;
    Move m = move_list[i];
    pos.make_move(m, st);

    int score = -negamax(pos, depth - 1);

    pos.unmake_move(st);

    if (score > best)
      best = score;
  }
  return best;
}