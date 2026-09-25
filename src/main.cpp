#include "book.h"
#include "movegen.h"
#include "terminal.h"
#include "uci.h"
#include "zobrist.h"
#include <cstring>

int main(int argc, char *argv[]) {
  init_movegen();
  init_zobrist();
  book_open("");

  if (argc >= 2 && std::strcmp(argv[1], "--terminal") == 0) {
    terminal_game();
  } else {
    uci_loop();
  }

  return 0;
}
