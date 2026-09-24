#ifndef RENJU_DECIDE_H
#define RENJU_DECIDE_H

#include "config.h"
#include "utils.h"
#include "board.h"
#include "analysis.h"
#include "tactics.h"
#include "eval.h"
#include "search.h"
#include "prooftree.h"
#include "negamax.h"

// ================= 8. 主决策 =================
bool _has_double_points_fb(FastBoard& fb, int color, const Deadline& deadline);

optional<Pos> choose_move(FastBoard& fb, int color, const Deadline& deadline,
    unordered_map<uint64_t, TTEntry>& vcf_tt,
    unordered_map<uint64_t, TTEntry>& vct_tt,
    unordered_map<uint64_t, TTEntry>& search_tt);

#endif
