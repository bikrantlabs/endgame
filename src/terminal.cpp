#include "evaluate.h"
#include "movegen.h"
#include "perft.h"
#include "position.h"
#include "types.h"
#include "utils.h"
#include <iostream>
#include <string>

int terminal_game() {
  Position pos;
  pos.set_startpos();
  perft_divide(pos, 5);

  std::cout << "\nSelect mode:\n";
  std::cout << "  1. Player vs Player\n";
  std::cout << "  2. Player vs AI (play as White)\n";
  std::cout << "> ";

  std::string mode_str;
  std::getline(std::cin, mode_str);
  int mode = (mode_str == "2") ? 2 : 1;

  while (true) {
    print_board(pos);
    Color turn = pos.side_to_move;

    bool is_ai_turn = (mode == 2 && turn == BLACK);

    if (is_ai_turn) {
      std::cout << "Black (AI) is thinking...\n";
      Move ai_move = gen_best_move(pos, 3);
      std::cout << "AI plays " << Util::move_to_string(ai_move) << "\n";
      StateInfo st;
      pos.make_move(ai_move, st);
      continue;
    }

    std::string from;
    std::string to;
    std::string turn_name = turn == 0 ? "White" : "Black";
    std::cout << "Select piece to move for " << turn_name << " : ";
    std::getline(std::cin, from);

    MoveList ml;
    if (from.size() == 2) {
      int from_square = parse_square(from);

      Color c = pos.color_on(from_square);

      gen_moves_for_square(pos, from_square, ml);

      PieceType pt = pos.piece_type_on(turn, from_square);
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

      if (is_promotion(move)) {
        std::string promo_piece;
        std::cout << "Promote to (n/b/r/q): ";
        std::getline(std::cin, promo_piece);

        int idx;
        if (promo_piece == "n")
          idx = 0;
        else if (promo_piece == "b")
          idx = 1;
        else if (promo_piece == "r")
          idx = 2;
        else
          idx = 3;

        MoveFlag base = is_capture(move) ? PROMO_CAP_N : PROMO_N;
        move = make_move(from_square, to_square,
                         static_cast<MoveFlag>(base + idx));
      }
      {
        StateInfo st;
        pos.make_move(move, st);
        int score = evaluate(pos);
        std::cout << "Score of " << Util::color_name(static_cast<Color>(1 - c))
                  << " is " << score << "\n";
      }

    } else {
      std::cout << "Please select a valid square: eg. e2\n";
    }
  }
}
