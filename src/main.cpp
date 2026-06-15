#include "movegen.h"
#include "terminal.h"
#include "uci.h"
#include <cstring>

int main(int argc, char *argv[]) {
  init_movegen();

  if (argc >= 2 && std::strcmp(argv[1], "--uci") == 0) {
    uci_loop();
  } else {
    terminal_game();
  }

  return 0;
}
