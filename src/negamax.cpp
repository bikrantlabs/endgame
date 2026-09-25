#include "negamax.h"
#include "material.h"
#include "movegen.h"
#include "order_moves.h"
#include "search.h"
#include "tt.h"
#include "types.h"
#include "utils.h"
#include "zobrist.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

// Convert score to/from TT format with distance-to-mate adjustment
static int value_to_tt(int score, int ply) {
  if (abs(score) > MATE_SCORE - MAX_PLY)
    return score > 0 ? score + ply : score - ply;
  return score;
}

static int value_from_tt(int score, int ply) {
  if (abs(score) > MATE_SCORE - MAX_PLY)
    return score > 0 ? score - ply : score + ply;
  return score;
}

int negamax(Position &pos, int depth, int alpha, int beta, int ply,
            Move *out_best_move, int *pv, int *pv_len,
            const Move *exclude, int exclude_count) {

  search_nodes++;

  if ((search_nodes & 2047) == 0) {
    if (time_is_up())
      search_stopped = true;
  }

  if (search_stopped)
    return 0;

  {
    Bitboard claimed = 0;
    Bitboard color_occ[COLOR_NB] = {0, 0};
    for (int c = 0; c < COLOR_NB; ++c)
      for (int pt = 0; pt < PIECE_TYPE_NB; ++pt) {
        if (pos.pieces[c][pt] & claimed) {
          Bitboard ov = pos.pieces[c][pt] & claimed;
          std::cerr << "[BOARDBAD] overlap at depth=" << depth << " ply=" << ply
                    << " c=" << c << " pt=" << pt << " sq=";
          while (ov) {
            int sq = unset_lsb(ov);
            std::cerr << sq << " ";
          }
          std::cerr << "\n";
          for (int cc = 0; cc < COLOR_NB; ++cc)
            for (int pp = 0; pp < PIECE_TYPE_NB; ++pp) {
              std::cerr << "  pieces[" << cc << "][" << pp << "]=" << std::hex
                        << pos.pieces[cc][pp] << std::dec << "\n";
            }
          std::cerr << "  occ[0]=" << std::hex << pos.occ[WHITE] << std::dec
                    << " occ[1]=" << std::hex << pos.occ[BLACK] << std::dec
                    << " all_occ=" << std::hex << pos.all_occ << std::dec
                    << "\n";
          dbg_path_dump();
          std::cerr << " side_to_move=" << pos.side_to_move
                    << " ep=" << pos.ep_square
                    << " castling=" << pos.castling_rights << "\n";
          print_board(pos, 0);
          std::exit(1);
        }
        claimed |= pos.pieces[c][pt];
        color_occ[c] |= pos.pieces[c][pt];
      }
    for (int c = 0; c < COLOR_NB; ++c) {
      if (pos.occ[c] != color_occ[c]) {
        std::cerr << "[BOARDBAD] occ mismatch depth=" << depth << " ply=" << ply
                  << " c=" << c << "\n";
        print_board(pos, 0);
        std::exit(1);
      }
    }
    if (pos.all_occ != (color_occ[WHITE] | color_occ[BLACK])) {
      std::cerr << "[BOARDBAD] all_occ mismatch depth=" << depth << " ply="
                << ply << "\n";
      print_board(pos, 0);
      std::exit(1);
    }
  }

  if (pv_len)
    pv_len[ply] = 0;

  ZobrishKey key = pos.hash;
  TTEntry *tte = tt.probe(key);
  Move tt_move = -1;
  if (tte) {
    tt_move = tte->best_move;
    if (tte->depth >= depth && ply > 0) {
      int tt_score = value_from_tt(tte->score, ply);
      if (tte->flag == TT_EXACT)
        return tt_score;
      if (tte->flag == TT_ALPHA && tt_score <= alpha)
        return alpha;
      if (tte->flag == TT_BETA && tt_score >= beta)
        return beta;
    }
  }

  // Null Move Pruning
  if (depth >= 3 && !pos.is_in_check() && pos.pieces[WHITE][QUEEN] != 0 &&
      pos.pieces[BLACK][QUEEN] != 0) {
    ZobrishKey saved_hash = pos.hash;
    int saved_ep = pos.ep_square;
    int saved_castling = pos.castling_rights;
    int saved_halfmove = pos.halfmove_clock;
    int saved_fullmove = pos.fullmove_number;

    // A null move passes the turn without moving a piece. Like a real move it
    // must clear the en-passant square, otherwise the flipped side would see
    // the opponent's EP target and generate invalid en-passant captures
    // (e.g. a white pawn on b2 "capturing" the a3 EP square left by its own
    // a2a4 double push). Such a bogus EP move removes a piece from the wrong
    // square and its unmake restores a phantom pawn on the source square.
    if (pos.ep_square != NO_SQUARE) {
      pos.hash ^= ZOBRIST.ep[sq_file(pos.ep_square)];
      pos.ep_square = NO_SQUARE;
    }
    pos.hash ^= ZOBRIST.side;
    pos.side_to_move = static_cast<Color>(1 - pos.side_to_move);

    // Save full board state: the recursive call will modify pieces[][]
    Bitboard saved_pieces[COLOR_NB][PIECE_TYPE_NB];
    std::copy(&pos.pieces[0][0],
              &pos.pieces[0][0] +
                  static_cast<int>(COLOR_NB) * static_cast<int>(PIECE_TYPE_NB),
              &saved_pieces[0][0]);
    Bitboard saved_occ[COLOR_NB] = {pos.occ[WHITE], pos.occ[BLACK]};
    Bitboard saved_all_occ = pos.all_occ;

    int reduction = 2 + depth / 4;
    int nd = depth - 1 - reduction;

    // Run the null-move subtree on scratch buffers so it never writes into
    // the real pv/pv_len arrays. It runs at the same ply and would otherwise
    // leave stale PV entries that corrupt the line up the tree.
    int null_pv[MAX_PLY];
    int null_pv_len[MAX_PLY];
    std::fill(null_pv_len, null_pv_len + MAX_PLY, 0);
    int score = -negamax(pos, nd, -beta, -beta + 1, ply, nullptr, null_pv,
                         null_pv_len, exclude, exclude_count);

    // Restore full board state
    std::copy(&saved_pieces[0][0],
              &saved_pieces[0][0] +
                  static_cast<int>(COLOR_NB) * static_cast<int>(PIECE_TYPE_NB),
              &pos.pieces[0][0]);
    pos.occ[WHITE] = saved_occ[WHITE];
    pos.occ[BLACK] = saved_occ[BLACK];
    pos.all_occ = saved_all_occ;
    pos.ep_square = saved_ep;
    pos.castling_rights = saved_castling;
    pos.halfmove_clock = saved_halfmove;
    pos.fullmove_number = saved_fullmove;
    pos.side_to_move = static_cast<Color>(1 - pos.side_to_move);
    pos.hash = saved_hash;

    if (score >= beta)
      return beta;
  }

  if (depth == 0)
    return quiescence(pos, alpha, beta);

  MoveList move_list;
  gen_all_moves(pos, move_list);

  if (move_list.empty()) {
    if (pos.is_in_check())
      return -MATE_SCORE + ply;
    return 0;
  }

  order_moves(pos, move_list, tt_move, ply);

  int best = -MATE_SCORE;
  Move best_move = 0;
  int orig_alpha = alpha;
  bool searched_any = false;

  for (int i = 0; i < move_list.count; i++) {
    if (search_stopped)
      break;

    if (ply == 0 && exclude_count > 0) {
      bool skip = false;
      for (int j = 0; j < exclude_count; j++) {
        if (move_list[i] == exclude[j]) {
          skip = true;
          break;
        }
      }
      if (skip)
        continue;
    }

    StateInfo st;
    Move m = move_list[i];
    pos.make_move(m, st);

    Color us = static_cast<Color>(1 - pos.side_to_move);
    bool king_captured = (pos.pieces[pos.side_to_move][KING] == 0);
    bool king_in_check = false;
    if (!king_captured)
      king_in_check =
          pos.is_square_attacked(Util::king_square(pos, us), pos.side_to_move);

    if (king_captured || king_in_check) {
      pos.unmake_move(st);
      continue;
    }

    searched_any = true;
    int score = -negamax(pos, depth - 1, -beta, -alpha, ply + 1, nullptr, pv,
                         pv_len, exclude, exclude_count);

    pos.repair_consistency();
    pos.unmake_move(st);

    if (score > best) {
      best = score;
      best_move = move_list[i];
      if (pv) {
        pv[ply] = best_move;
        pv_len[ply] = 1 + pv_len[ply + 1];
      }
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

  if (!searched_any) {
    if (search_stopped)
      return 0;
    if (pos.is_in_check())
      return -MATE_SCORE + ply;
    return 0; // stalemate
  }

  if (best <= -MATE_SCORE) {
    // Fail-low node: every child scored at or below the sentinel. True value
    // is bounded above by orig_alpha, so return that bound (never the raw
    // -MATE_SCORE sentinel, which would negate into a fake mate score at the
    // parent) and skip the TT store to avoid poisoning later searches.
    return orig_alpha;
  }

  TTFlag flag;
  if (best <= orig_alpha)
    flag = TT_ALPHA;
  else if (best >= beta)
    flag = TT_BETA;
  else
    flag = TT_EXACT;
  tt.store(key, depth, value_to_tt(best, ply), flag, best_move);

  if (out_best_move)
    *out_best_move = best_move;
  return best;
}
