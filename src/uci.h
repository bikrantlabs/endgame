#pragma once
#include "position.h"
#include "types.h"
#include <atomic>
#include <climits>

extern std::atomic<bool> search_stopped;
extern uint64_t search_nodes;

struct SearchLimits {
  int depth = 64;
  int movetime = 0;
  int wtime = 0, btime = 0, winc = 0, binc = 0;
  int movestogo = 0;
  bool infinite = false;

  int time_for_move(Color side, int elapsed) const;
};

void uci_loop();
