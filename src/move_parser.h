#pragma once
#include "position.h"
#include <string>

Move parse_uci_move(Position &pos, const std::string &str);

bool set_fen(Position &pos, const std::string &fen);