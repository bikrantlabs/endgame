#include "perft.h"
#include "movegen.h"
#include "position.h"
#include "utils.h"
#include <iostream>

uint64_t perft(Position &pos, int depth) {

  if (depth == 0)
    return 1;

  MoveList moves;
  gen_legal_moves(pos, moves);

  uint64_t nodes = 0;

  for (int i = 0; i < moves.count; i++) {

    Move m = moves[i];

    StateInfo st;

    pos.make_move(m, st);

    nodes += perft(pos, depth - 1);

    pos.unmake_move(st);
  }

  return nodes;
}

void perft_divide(Position &pos, int depth) {

  MoveList moves;
  gen_legal_moves(pos, moves);

  uint64_t total = 0;

  for (int i = 0; i < moves.count; i++) {

    Move m = moves[i];

    StateInfo st;
    pos.make_move(m, st);

    uint64_t nodes = perft(pos, depth - 1);

    pos.unmake_move(st);

    std::cout << Util::move_to_string(m) << ": " << nodes << "\n";

    total += nodes;
  }

  std::cout << "\nTotal: " << total << "\n";
}