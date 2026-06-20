#include "uci.h"
#include "movegen.h"
#include "negamax.h"
#include "position.h"
#include "types.h"
#include "utils.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

std::atomic<bool> search_stopped{false};
uint64_t search_nodes{0};

// ── FEN parser ──────────────────────────────────────────────────────────────

static int fen_char_to_piece(char c) {
  switch (c) {
  case 'P':
    return W_PAWN;
  case 'N':
    return W_KNIGHT;
  case 'B':
    return W_BISHOP;
  case 'R':
    return W_ROOK;
  case 'Q':
    return W_QUEEN;
  case 'K':
    return W_KING;
  case 'p':
    return B_PAWN;
  case 'n':
    return B_KNIGHT;
  case 'b':
    return B_BISHOP;
  case 'r':
    return B_ROOK;
  case 'q':
    return B_QUEEN;
  case 'k':
    return B_KING;
  default:
    return -1;
  }
}

static bool set_fen(Position &pos, const std::string &fen) {
  pos.clear();

  // Split FEN into fields
  std::istringstream ss(fen);
  std::string board, turn, castling, ep, hc, fn;
  ss >> board >> turn >> castling >> ep >> hc >> fn;

  // Piece placement
  int sq = 56; // a8
  for (char c : board) {
    if (c == '/') {
      sq -= 16; // back to a-file, one rank down
      continue;
    }
    if (c >= '1' && c <= '8') {
      sq += (c - '0');
      continue;
    }
    int piece = fen_char_to_piece(c);
    if (piece < 0)
      return false;
    Color color = (piece < B_PAWN) ? WHITE : BLACK;
    PieceType pt =
        static_cast<PieceType>((piece < B_PAWN) ? piece : piece - B_PAWN);
    pos.place_piece(color, pt, sq);
    sq++;
  }

  // Side to move
  pos.side_to_move = (turn == "b") ? BLACK : WHITE;

  // Castling rights
  pos.castling_rights = 0;
  for (char c : castling) {
    if (c == 'K')
      pos.castling_rights |= WK_CASTLE;
    else if (c == 'Q')
      pos.castling_rights |= WQ_CASTLE;
    else if (c == 'k')
      pos.castling_rights |= BK_CASTLE;
    else if (c == 'q')
      pos.castling_rights |= BQ_CASTLE;
  }

  // En passant
  if (ep != "-") {
    pos.ep_square = parse_square(ep);
  } else {
    pos.ep_square = NO_SQUARE;
  }

  // Clocks
  pos.halfmove_clock = std::atoi(hc.c_str());
  pos.fullmove_number = std::atoi(fn.c_str());

  // Rebuild occupancy
  pos.occ[WHITE] = pos.occ[BLACK] = pos.all_occ = 0;
  for (int pt = 0; pt < PIECE_TYPE_NB; ++pt) {
    pos.occ[WHITE] |= pos.pieces[WHITE][pt];
    pos.occ[BLACK] |= pos.pieces[BLACK][pt];
  }
  pos.all_occ = pos.occ[WHITE] | pos.occ[BLACK];

  pos.compute_hash();
  pos.game_history.clear();
  pos.game_history.push_back(pos.hash);
  return true;
}

// ── UCI move parser ─────────────────────────────────────────────────────────

static Move parse_uci_move(Position &pos, const std::string &str) {
  if (str.size() < 4)
    return Move{};

  int from = parse_square(str.substr(0, 2));
  int to = parse_square(str.substr(2, 2));
  if (from == NO_SQUARE || to == NO_SQUARE)
    return Move{};

  MoveList ml;
  gen_legal_moves(pos, ml);

  for (int i = 0; i < ml.count; i++) {
    if (move_from(ml[i]) == from && move_to(ml[i]) == to) {
      // Promotion
      if (str.size() >= 5) {
        char promo = str[4];
        MoveFlag wanted;
        if (promo == 'n')
          wanted = is_capture(ml[i]) ? PROMO_CAP_N : PROMO_N;
        else if (promo == 'b')
          wanted = is_capture(ml[i]) ? PROMO_CAP_B : PROMO_B;
        else if (promo == 'r')
          wanted = is_capture(ml[i]) ? PROMO_CAP_R : PROMO_R;
        else
          wanted = is_capture(ml[i]) ? PROMO_CAP_Q : PROMO_Q;
        if (move_flag(ml[i]) == wanted)
          return ml[i];
      } else {
        return ml[i];
      }
    }
  }
  return Move{};
}

// ── Search ──────────────────────────────────────────────────────────────────

static void iterative_deepening(Position &pos, SearchLimits &limits) {
  search_stopped = false;
  search_nodes = 0;

  auto start = std::chrono::steady_clock::now();
  Move best_move{};
  Move current_root{};

  for (int depth = 1; depth <= limits.depth; depth++) {
    if (search_stopped)
      break;

    MoveList ml;
    gen_legal_moves(pos, ml);
    if (ml.empty())
      break;

    int alpha = INT_MIN + 1;
    int beta = INT_MAX;
    int best_score = INT_MIN + 1;

    // Search at root — pick best move
    for (int i = 0; i < ml.count; i++) {
      if (search_stopped)
        break;

      StateInfo st;
      Move m = ml[i];
      pos.make_move(m, st);

      int score = -negamax(pos, depth - 1, -beta, -alpha, 0);

      pos.unmake_move(st);

      if (score > best_score) {
        best_score = score;
        best_move = m;
      }
      if (score > alpha)
        alpha = score;
    }

    // Time check
    auto now = std::chrono::steady_clock::now();
    int elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
            .count();

    // UCI info
    std::cout << "info depth " << depth << " score cp " << best_score
              << " nodes " << search_nodes << " time " << elapsed << " pv "
              << Util::move_to_string(best_move) << "\n";

    // Stop if allocated time exceeded
    int time_alloc = limits.time_for_move(pos.side_to_move, elapsed);
    if (elapsed >= time_alloc && depth > 1)
      break;
  }

  auto end = std::chrono::steady_clock::now();
  int total_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();
  std::cout << "bestmove " << Util::move_to_string(best_move) << "\n";
}

// ── Search limits ───────────────────────────────────────────────────────────

static SearchLimits parse_go(const std::string &line) {
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

int SearchLimits::time_for_move(Color side, int elapsed) const {
  if (movetime > 0)
    return movetime;
  if (infinite)
    return INT_MAX;

  int time_remaining = (side == WHITE) ? wtime : btime;
  int inc = (side == WHITE) ? winc : binc;

  if (time_remaining == 0)
    return INT_MAX;

  // Use 1/40 of remaining time + increment
  int alloc = time_remaining / 40 + inc;
  if (alloc < 100)
    alloc = 100; // minimum 100ms
  if (alloc > time_remaining / 2)
    alloc = time_remaining / 2; // never use more than half of remaining

  return alloc;
}

// ── UCI loop ────────────────────────────────────────────────────────────────

void uci_loop() {
  Position pos;
  pos.set_startpos();

  std::cout.setf(std::ios::unitbuf);

  std::string line;
  while (std::getline(std::cin, line)) {
    // Trim whitespace
    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    if (line == "uci") {
      std::cout << "id name Endgame\n";
      std::cout << "id author Endgame\n";
      std::cout << "uciok\n";
    } else if (line == "isready") {
      std::cout << "readyok\n";
    } else if (line == "ucinewgame") {
      // no-op
    } else if (line.rfind("position", 0) == 0) {
      // position [startpos | fen <fen>] [moves <move> ...]
      std::string rest = line.substr(9); // after "position "

      if (rest.rfind("startpos", 0) == 0) {
        pos.set_startpos();
        rest = rest.substr(8); // consume "startpos"
      } else if (rest.rfind("fen", 0) == 0) {
        rest = rest.substr(4); // consume "fen "
        // Find the moves part
        auto moves_pos = rest.find(" moves ");
        std::string fen_part =
            (moves_pos == std::string::npos) ? rest : rest.substr(0, moves_pos);
        set_fen(pos, fen_part);
        rest =
            (moves_pos == std::string::npos) ? "" : rest.substr(moves_pos + 7);
      }

      // Apply moves
      auto moves_pos = line.find(" moves ");
      if (moves_pos != std::string::npos) {
        std::string moves_str = line.substr(moves_pos + 7);
        std::istringstream ms(moves_str);
        std::string mstr;
        while (ms >> mstr) {
          Move m = parse_uci_move(pos, mstr);
          if (m != Move{}) {
            StateInfo st;
            pos.make_move(m, st);
          }
        }
      }
    } else if (line.rfind("go", 0) == 0) {
      SearchLimits limits = parse_go(line);
      iterative_deepening(pos, limits);

    } else if (line == "stop") {
      search_stopped = true;
    } else if (line == "quit") {
      search_stopped = true;
      break;
    }
  }
}
