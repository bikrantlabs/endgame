#pragma once
#include <cstdint>
#include <random>

struct ZobristTable {
  uint64_t pieces[12][64]; // [piece_index 0-11][square 0-63]
  uint64_t side;           // flip on every move
  uint64_t castling[16];   // all 4-bit combos of KQkq rights
  uint64_t
      ep[8]; // en_passant: indexed by file (only when EP is actually available)
};

inline ZobristTable ZOBRIST;

// Fill every single slots with random keys.
inline void init_zobrist() {
  std::mt19937_64 rng(1070372); // fixed seed = reproducible
  auto r = [&] { return rng(); };

  for (auto &arr : ZOBRIST.pieces)
    for (auto &v : arr)
      v = r();
  ZOBRIST.side = r();
  for (auto &v : ZOBRIST.castling)
    v = r();
  for (auto &v : ZOBRIST.ep)
    v = r();
}