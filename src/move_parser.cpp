#include "move_parser.h"
#include "movegen.h"
#include "types.h"
#include <cstdlib>
#include <sstream>

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

bool set_fen(Position &pos, const std::string &fen) {
  pos.clear();

  std::istringstream ss(fen);
  std::string board, turn, castling, ep, hc, fn;
  ss >> board >> turn >> castling >> ep >> hc >> fn;

  // Piece placement
  int sq = 56;
  for (char c : board) {
    if (c == '/') {
      sq -= 16;
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

  pos.side_to_move = (turn == "b") ? BLACK : WHITE;

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

  if (ep != "-") {
    pos.ep_square = parse_square(ep);
  } else {
    pos.ep_square = NO_SQUARE;
  }

  pos.halfmove_clock = std::atoi(hc.c_str());
  pos.fullmove_number = std::atoi(fn.c_str());

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

Move parse_uci_move(Position &pos, const std::string &str) {
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
