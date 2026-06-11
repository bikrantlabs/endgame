#pragma once

#include <cstdint>

class Position;

uint64_t perft(Position &pos, int depth);
void perft_divide(Position &pos, int depth);