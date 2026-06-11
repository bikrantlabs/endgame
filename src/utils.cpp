#include "utils.h"

#include "types.h"
#include <iostream>

// Declare the namespace
namespace Util {

int king_square(const Position &pos, Color c) {
  return lsb(pos.pieces[c][KING]);
}

const char *piece_name(PieceType pt) {
  const char *names[] = {"Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};
  return (pt < PIECE_TYPE_NB) ? names[pt] : "None";
}
std::string move_to_string(Move m) {

  int from = move_from(m);
  int to = move_to(m);
  int flag = move_flag(m);

  std::string s;

  // basic move: e2e4
  s += sq_name(from);
  s += sq_name(to);

  // promotions
  if (flag >= PROMO_N) {

    switch (flag) {
    case PROMO_N:
    case PROMO_CAP_N:
      s += 'n';
      break;

    case PROMO_B:
    case PROMO_CAP_B:
      s += 'b';
      break;

    case PROMO_R:
    case PROMO_CAP_R:
      s += 'r';
      break;

    case PROMO_Q:
    case PROMO_CAP_Q:
      s += 'q';
      break;
    }
  }

  // optional: make castling readable
  if (flag == KING_CASTLE) {
    return "O-O";
  }
  if (flag == QUEEN_CASTLE) {
    return "O-O-O";
  }

  return s;
}
const char *color_name(Color c) { return c == 0 ? "WHITE" : "BLACK"; }

// Find the move in the legal list that matches from+to, return 0 if not found
Move find_move(const MoveList &legal, int from, int to) {

  for (int i = 0; i < legal.count; i++) {
    if ((from == move_from(legal[i]) && (to == move_to(legal[i]))))
      return legal[i];
  }
  std::cout << "Find move returning 0";
  return 0;
}

void print_targets(const MoveList &ml) {
  if (ml.empty()) {
    std::cout << "  (No legal moves available)\n";
    return;
  }
  for (int i = 0; i < ml.count; ++i) {
    std::cout << "  " << i + 1 << ") " << sq_name(move_to(ml.moves[i])) << "\n";
  }
}
} // namespace Util
