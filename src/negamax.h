#pragma once

#include "position.h"
int negamax(Position &pos, int depth, int alpha, int beta, int ply,
            Move *out_best_move = nullptr, int *pv = nullptr,
            int *pv_len = nullptr, const Move *exclude = nullptr,
            int exclude_count = 0);
