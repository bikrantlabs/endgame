#pragma once
#include "position.h"
#include "types.h" // for MoveList
#include "types.h"

constexpr int MAX_PLY = 128;

// killer moves: 2 per ply
extern Move killer_moves[MAX_PLY][2];

// history heuristic: indexed by [from_square][to_square]
extern int history_table[64][64];

// MVV_LVA[attacker][victim]
constexpr int MVV_LVA[6][6] = {
    // victim:       PAWN  KNIGHT BISHOP  ROOK  QUEEN  KING
    /* attacker PAWN   */ {15, 25, 35, 45, 55, 0},
    /* attacker KNIGHT */ {14, 24, 34, 44, 54, 0},
    /* attacker BISHOP */ {13, 23, 33, 43, 53, 0},
    /* attacker ROOK   */ {12, 22, 32, 42, 52, 0},
    /* attacker QUEEN  */ {11, 21, 31, 41, 51, 0},
    /* attacker KING   */ {10, 20, 30, 40, 50, 0},
};

void store_killer(Move m, int ply);
void update_history(Move m, int depth);
void clear_search_tables(); // call once per new search (gen_best_move)
int quiescence(Position &pos, int alpha, int beta);

void order_moves(Position &pos, MoveList &move_list, Move tt_move, int ply);