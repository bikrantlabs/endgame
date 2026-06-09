#include "utils.h"

#include "types.h"
#include <iostream>

// Declare the namespace
namespace Util {

const char *piece_name(PieceType pt) {
  const char *names[] = {"Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};
  return (pt < PIECE_TYPE_NB) ? names[pt] : "None";
}

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
