#include "search.h"
#include "negamax.h"
#include "utils.h"
#include <chrono>
#include <iostream>
#include <sstream>

std::chrono::steady_clock::time_point search_start_time;
int search_time_limit_ms = 0; // 0 = no limit

int SearchLimits::time_for_move(Color side, int elapsed) const {
  if (movetime > 0)
    return movetime;
  if (infinite)
    return 0; // 0 = no limit, see time_is_up()

  int time_remaining = (side == WHITE) ? wtime : btime;
  int inc = (side == WHITE) ? winc : binc;

  if (time_remaining == 0)
    return 0; // no clock info at all — no limit

  int alloc = time_remaining / 40 + inc;
  if (alloc < 100)
    alloc = 100;
  if (alloc > time_remaining / 2)
    alloc = time_remaining / 2;

  return alloc;
}

SearchLimits parse_go(const std::string &line) {
  SearchLimits lim;
  std::istringstream ss(line);
  std::string token;
  ss >> token; // skip "go"

  while (ss >> token) {
    if (token == "depth")
      ss >> lim.depth;
    else if (token == "movetime")
      ss >> lim.movetime;
    else if (token == "wtime")
      ss >> lim.wtime;
    else if (token == "btime")
      ss >> lim.btime;
    else if (token == "winc")
      ss >> lim.winc;
    else if (token == "binc")
      ss >> lim.binc;
    else if (token == "movestogo")
      ss >> lim.movestogo;
    else if (token == "infinite")
      lim.infinite = true;
  }
  return lim;
}

void iterative_deepening(Position &pos, SearchLimits &limits) {
  search_stopped = false;
  search_nodes = 0;

  search_start_time = std::chrono::steady_clock::now();
  search_time_limit_ms = limits.time_for_move(pos.side_to_move, 0);

  Move best_move{};
  int best_score = 0;

  for (int depth = 1; depth <= limits.depth; depth++) {
    if (search_stopped)
      break;

    Move root_best{};
    int score = negamax(pos, depth, INT_MIN + 1, INT_MAX, 0, &root_best);

    if (search_stopped) {
      // This depth was interrupted mid-search — its result is unreliable.
      // Keep the previous fully-completed depth's best_move instead.
      break;
    }

    best_move = root_best;
    best_score = score;

    auto now = std::chrono::steady_clock::now();
    int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now - search_start_time)
                      .count();

    std::cout << "info depth " << depth << " score cp " << best_score
              << " nodes " << search_nodes << " time " << elapsed << " pv "
              << Util::move_to_string(best_move) << "\n";

    // Stop if this depth alone already used up the budget
    if (search_time_limit_ms > 0 && elapsed >= search_time_limit_ms &&
        depth > 1)
      break;
  }

  std::cout << "bestmove " << Util::move_to_string(best_move) << "\n";
}