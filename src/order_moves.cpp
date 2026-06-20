#include "order_moves.h"
#include "evaluate.h"
#include "material.h" // wherever piece_value lives
#include "movegen.h"
#include "types.h"
#include <algorithm>
#include <cstring>

Move killer_moves[MAX_PLY][2];
int history_table[64][64];

void clear_search_tables() {
  std::memset(
      killer_moves, -1,
      sizeof(killer_moves)); // -1 == no move, matches your NO_MOVE convention
  std::memset(history_table, 0, sizeof(history_table));
}

void store_killer(Move m, int ply) {
  if (ply >= MAX_PLY)
    return;
  if (killer_moves[ply][0] == m)
    return; // already top killer, skip
  killer_moves[ply][1] = killer_moves[ply][0];
  killer_moves[ply][0] = m;
}

void update_history(Move m, int depth) {
  int from = move_from(m);
  int to = move_to(m);
  history_table[from][to] += depth * depth; // deeper cutoffs weighted more
}

static int score_move(Position &pos, Move m, Move tt_move, int ply) {
  if (m == tt_move)
    return 1'000'000;

  if (is_capture(m)) {
    int to = move_to(m);
    int from = move_from(m);
    Color attacker_color = pos.color_on(from);
    PieceType attacker_pt = pos.piece_type_on(attacker_color, from);

    PieceType victim_pt;
    if (move_flag(m) == EP_CAPTURE) {
      victim_pt = PAWN; // EP is always pawn-takes-pawn
    } else {
      Color victim_color = pos.color_on(to);
      victim_pt = pos.piece_type_on(victim_color, to);
    }

    return 100'000 + MVV_LVA[attacker_pt][victim_pt];
  }

  if (m == killer_moves[ply][0])
    return 90'000;
  if (m == killer_moves[ply][1])
    return 89'000;

  return history_table[move_from(m)][move_to(m)];
}

void order_moves(Position &pos, MoveList &move_list, Move tt_move, int ply) {
  int scores[256];
  for (int i = 0; i < move_list.count; i++)
    scores[i] = score_move(pos, move_list[i], tt_move, ply);

  // insertion sort, descending by score
  for (int i = 1; i < move_list.count; i++) {
    int j = i;
    while (j > 0 && scores[j - 1] < scores[j]) {
      std::swap(scores[j - 1], scores[j]);
      std::swap(move_list.moves[j - 1], move_list.moves[j]);
      j--;
    }
  }
}

int quiescence(Position &pos, int alpha, int beta) {
  int stand_pat = evaluate(pos);

  if (stand_pat >= beta)
    return beta; // already too good, opponent won't allow this — fail high

  if (stand_pat > alpha)
    alpha = stand_pat; // this is our "do nothing" baseline

  MoveList captures;
  gen_capture_moves(
      pos,
      captures); // ONLY captures TODO: (and maybe promotions/checks)

  order_moves(pos, captures, /*tt_move=*/-1,
              /*ply=*/0); // MVV-LVA ordering matters a lot here

  for (int i = 0; i < captures.count; i++) {
    StateInfo st;
    Move m = captures[i];
    pos.make_move(m, st);
    int score = -quiescence(pos, -beta, -alpha);
    pos.unmake_move(st);

    if (score >= beta)
      return beta;
    if (score > alpha)
      alpha = score;
  }

  return alpha;
}