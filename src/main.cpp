#include "movegen.h"
#include "perft.h"
#include "position.h"
#include "types.h"
#include "utils.h"
#include <iostream>
#include <string>

int main() {
  init_movegen();
  Position pos;
  pos.set_startpos();
  perft_divide(pos, 6);
  // manually replicate the position

  while (true) {
    print_board(pos);
    std::string from;
    std::string to;
    Color turn = pos.side_to_move;

    std::string turn_name = turn == 0 ? "White" : "Black";
    std::cout << "Select piece to move for " << turn_name << " : ";
    std::getline(std::cin, from);

    MoveList ml;
    if (from.size() == 2) {
      int from_square = parse_square(from);

      Color c = pos.color_on(from_square);

      // Get all moves for that square
      gen_moves_for_square(pos, from_square, ml);

      PieceType pt = pos.piece_on(turn, from_square);
      std::cout << "\n Target " << Util::color_name(c) << Util::piece_name(pt)
                << " can move to: \n";
      Util::print_targets(ml);
      std::cout << "\n Where do you want to move " << from << " :";

      getline(std::cin, to);
      int to_square = parse_square(to);

      Move move = Util::find_move(ml, from_square, to_square);

      if (move == 0) {
        std::cout << "No legal move from " << from_square << " to "
                  << to_square;
        continue;
      }
      {
        StateInfo st;
        pos.make_move(move, st);
      }

    } else {
      std::cout << "Please select a valid square: eg. e2\n";
    }
  }
}
