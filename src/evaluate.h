#pragma once

#include "position.h"

int evaluate(const Position &pos);
int evaluate_material(const Position &pos, int phase);
int evaluate_pst(const Position &pos, int phase);
int game_phase(const Position &pos);
