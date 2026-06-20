#pragma once
#include "types.h"
#include <cstdint>
#include <vector>
enum TTFlag { TT_EXACT, TT_ALPHA, TT_BETA };

struct TTEntry {
  uint64_t key;       // Full Zobrist hash (to detect collisions)
  int score;          // Stored score
  Move best_move = 0; // Best move from this position
  int depth = -1;     // Depth this was searched at
  TTFlag flag; // Exact, lower bound (beta cutoff), or upper bound (all-node)
};

struct TranspositionTable {
  std::vector<TTEntry> table;
  size_t size;

  TranspositionTable(size_t mb = 100) {
    size = (mb * 1024 * 1024) / sizeof(TTEntry);
    table.resize(size);
  }

  TTEntry *probe(uint64_t key) {
    TTEntry &e = table[key % size];
    if (e.key == key)
      return &e;
    return nullptr;
  }

  void store(uint64_t key, int depth, int score, TTFlag flag, Move best) {
    TTEntry &e = table[key % size];
    // simple replacement scheme: always replace, or replace if deeper
    if (depth >= e.depth || e.key != key) {
      e.key = key;
      e.depth = depth;
      e.score = score;
      e.flag = flag;
      e.best_move = best;
    }
  }
};

inline TranspositionTable tt(64);