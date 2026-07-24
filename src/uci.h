#pragma once
#include "types.h"
#include <string>

struct UCIOptions {
  bool chess960 = false;
  int hash_size = 100;
  std::string book_file = "book.bin";
};

extern UCIOptions uci_options;

void uci_loop();
