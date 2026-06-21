#include "search.h"
#include "material.h"
#include "negamax.h"
#include "utils.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <sstream>

std::chrono::steady_clock::time_point search_start_time;
int search_time_limit_ms = 0;

std::atomic<bool> search_stopped{false};
uint64_t search_nodes{0};

int SearchLimits::time_for_move(Color side, int elapsed) const {
  if (movetime > 0)
    return movetime;
  if (infinite)
    return 0;

  int time_remaining = (side == WHITE) ? wtime : btime;
  int inc = (side == WHITE) ? winc : binc;

  if (time_remaining == 0)
    return 0;

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
  ss >> token;

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

  // Save root position to repair child-search corruption between depths
  Bitboard snapshot_pieces[COLOR_NB][PIECE_TYPE_NB];
  std::copy(&pos.pieces[0][0], &pos.pieces[0][0] + static_cast<int>(COLOR_NB) * static_cast<int>(PIECE_TYPE_NB), &snapshot_pieces[0][0]);
  Bitboard snapshot_occ[COLOR_NB] = {pos.occ[WHITE], pos.occ[BLACK]};
  ZobrishKey snapshot_hash = pos.hash;
  int snapshot_castling = pos.castling_rights;
  int snapshot_ep = pos.ep_square;
  int snapshot_halfmove = pos.halfmove_clock;
  int snapshot_fullmove = pos.fullmove_number;

  for (int depth = 1; depth <= limits.depth; depth++) {
    if (search_stopped)
      break;

    // Restore clean root position before each depth
    std::copy(&snapshot_pieces[0][0], &snapshot_pieces[0][0] + static_cast<int>(COLOR_NB) * static_cast<int>(PIECE_TYPE_NB), &pos.pieces[0][0]);
    pos.occ[WHITE] = snapshot_occ[WHITE];
    pos.occ[BLACK] = snapshot_occ[BLACK];
    pos.all_occ = snapshot_occ[WHITE] | snapshot_occ[BLACK];
    pos.hash = snapshot_hash;
    pos.castling_rights = snapshot_castling;
    pos.ep_square = snapshot_ep;
    pos.halfmove_clock = snapshot_halfmove;
    pos.fullmove_number = snapshot_fullmove;

    Move root_best{};
    int score = negamax(pos, depth, -MATE_SCORE, MATE_SCORE, 0, &root_best);

    if (search_stopped)
      break;

    best_move = root_best;
    best_score = score;

    auto now = std::chrono::steady_clock::now();
    int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now - search_start_time)
                      .count();

    std::cout << "info depth " << depth << " score cp " << best_score
              << " nodes " << search_nodes << " time " << elapsed << " pv "
              << Util::move_to_string(best_move) << "\n";

    if (search_time_limit_ms > 0 && elapsed >= search_time_limit_ms &&
        depth > 1)
      break;
  }

  std::cout << "bestmove " << Util::move_to_string(best_move) << "\n";
}
