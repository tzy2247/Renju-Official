#ifndef RENJU_EVAL_H
#define RENJU_EVAL_H

#include "config.h"
#include "utils.h"
#include "board.h"
#include "analysis.h"
#include "tactics.h"

// ================= 5. 评估 =================
double _attack_val(FastBoard& fb, int x, int y, int color);
double quick_eval_fb(FastBoard& fb, int x, int y, int color);
double evaluate_fb(FastBoard& fb, int tomove);
optional<Pos> _best_by_eval_fb(FastBoard& fb, const vector<Pos>& moves, int color);
optional<Pos> fallback_move_fb(FastBoard& fb, int color);
vector<Pos> top_moves_fb(FastBoard& fb, int color, int n);

#endif
