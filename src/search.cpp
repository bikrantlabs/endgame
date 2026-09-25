#include "search.h"
#include "material.h"
#include "movegen.h"
#include "negamax.h"
#include "order_moves.h"
#include "position.h"
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
    else if (token == "multipv")
      ss >> lim.multipv;
  }
  return lim;
}

static std::string format_score(int score) {
  if (score > MATE_SCORE - MAX_PLY) {
    int m = (MATE_SCORE - score + 1) / 2;
    return "mate " + std::to_string(m);
  }
  if (score < -MATE_SCORE + MAX_PLY) {
    int m = (MATE_SCORE + score + 1) / 2;
    return "mate -" + std::to_string(m);
  }
  return "cp " + std::to_string(score);
}

struct RootMove {
  Move move = 0;
  int score = 0;
  int pv[MAX_PLY];
  int pv_len = 0;
};

void iterative_deepening(Position &pos, SearchLimits &limits, Move book_move) {
  search_stopped = false;
  search_nodes = 0;

  search_start_time = std::chrono::steady_clock::now();
  search_time_limit_ms = limits.time_for_move(pos.side_to_move, 0);

  Move best_move{};
  Move ponder_move{};

  dbg_path_reset();

  int multipv = limits.multipv < 1 ? 1 : limits.multipv;
  if (multipv > MAX_PLY)
    multipv = MAX_PLY;

  // Save root position to repair child-search corruption between depths
  Bitboard snapshot_pieces[COLOR_NB][PIECE_TYPE_NB];
  std::copy(&pos.pieces[0][0], &pos.pieces[0][0] + static_cast<int>(COLOR_NB) * static_cast<int>(PIECE_TYPE_NB), &snapshot_pieces[0][0]);
  Bitboard snapshot_occ[COLOR_NB] = {pos.occ[WHITE], pos.occ[BLACK]};
  ZobrishKey snapshot_hash = pos.hash;
  int snapshot_castling = pos.castling_rights;
  int snapshot_ep = pos.ep_square;
  int snapshot_halfmove = pos.halfmove_clock;
  int snapshot_fullmove = pos.fullmove_number;

  auto restore_root = [&]() {
    std::copy(&snapshot_pieces[0][0], &snapshot_pieces[0][0] + static_cast<int>(COLOR_NB) * static_cast<int>(PIECE_TYPE_NB), &pos.pieces[0][0]);
    pos.occ[WHITE] = snapshot_occ[WHITE];
    pos.occ[BLACK] = snapshot_occ[BLACK];
    pos.all_occ = snapshot_occ[WHITE] | snapshot_occ[BLACK];
    pos.hash = snapshot_hash;
    pos.castling_rights = snapshot_castling;
    pos.ep_square = snapshot_ep;
    pos.halfmove_clock = snapshot_halfmove;
    pos.fullmove_number = snapshot_fullmove;
  };

  for (int depth = 1; depth <= limits.depth; depth++) {
    if (search_stopped)
      break;

    RootMove slots[MAX_PLY];
    int slot_count = 0;

    for (int slot = 0; slot < multipv; slot++) {
      restore_root();

      Move exclude[MAX_PLY];
      for (int j = 0; j < slot; j++)
        exclude[j] = slots[j].move;

      Move root_best{};
      int pv[MAX_PLY];
      int pv_len[MAX_PLY];
      std::fill(pv_len, pv_len + MAX_PLY, 0);

      int score = negamax(pos, depth, -MATE_SCORE, MATE_SCORE, 0, &root_best,
                          pv, pv_len, exclude, slot);

      if (search_stopped)
        break;

      if (root_best == 0)
        break; // no legal move left (mate/stalemate or all excluded)

      slots[slot].move = root_best;
      slots[slot].score = score;
      slots[slot].pv_len = pv_len[0];
      for (int k = 0; k < slots[slot].pv_len; k++)
        slots[slot].pv[k] = pv[k];
      slot_count = slot + 1;
    }

    if (search_stopped)
      break;

    std::sort(slots, slots + slot_count,
              [](const RootMove &a, const RootMove &b) {
                return a.score > b.score;
              });

    auto now = std::chrono::steady_clock::now();
    int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now - search_start_time)
                      .count();

    for (int s = 0; s < slot_count; s++) {
      if (slots[s].move == 0)
        continue;
      std::cout << "info depth " << depth << " multipv " << (s + 1)
                << " score " << format_score(slots[s].score) << " nodes "
                << search_nodes << " time " << elapsed << " pv ";
      for (int k = 0; k < slots[s].pv_len; k++) {
        std::cout << Util::move_to_string(slots[s].pv[k]);
        if (k + 1 < slots[s].pv_len)
          std::cout << " ";
      }
      std::cout << "\n";
    }

    best_move = slots[0].move;
    ponder_move = (slots[0].pv_len > 1) ? slots[0].pv[1] : 0;

    if (search_time_limit_ms > 0 && elapsed >= search_time_limit_ms &&
        depth > 1)
      break;
  }

  // Book move is authoritative when present: it keeps opening variety and
  // sanity even though the shallow search may "refute" it with a bad eval.
  // PV info lines above still come from the real search. For a book override,
  // run a short continuation search to produce a legal ponder reply.
  if (book_move != 0) {
    best_move = book_move;
    ponder_move = 0;
    if (!search_stopped) {
      Position cont = pos;
      StateInfo st;
      cont.make_move(book_move, st);
      Move root_best = 0;
      int pv[MAX_PLY];
      int pv_len[MAX_PLY];
      std::fill(pv_len, pv_len + MAX_PLY, 0);
      negamax(cont, 4, -MATE_SCORE, MATE_SCORE, 0, &root_best, pv, pv_len,
              nullptr, 0);
      ponder_move = root_best;
    }
  }

  if (best_move == 0) {
    std::cout << "bestmove (none)\n";
  } else {
    std::cout << "bestmove " << Util::move_to_string(best_move);
    if (ponder_move != 0)
      std::cout << " ponder " << Util::move_to_string(ponder_move);
    std::cout << "\n";
  }
}
