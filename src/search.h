// search.h
#pragma once
#include "position.h"
#include <atomic>
#include <chrono>

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

SearchLimits parse_go(const std::string &line);
void iterative_deepening(Position &pos, SearchLimits &limits);

// search.cpp / search.h additions
extern std::chrono::steady_clock::time_point search_start_time;
extern int search_time_limit_ms;

inline bool time_is_up() {
  if (search_time_limit_ms <= 0)
    return false; // infinite/no limit
  auto now = std::chrono::steady_clock::now();
  int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - search_start_time)
                    .count();
  return elapsed >= search_time_limit_ms;
}