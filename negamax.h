#ifndef RENJU_NEGAMAX_H
#define RENJU_NEGAMAX_H

#include "config.h"
#include "utils.h"
#include "board.h"
#include "analysis.h"
#include "tactics.h"
#include "eval.h"
#include "search.h"

// ================= 7. Negamax =================
pair<double, optional<Pos>> negamax(FastBoard& fb, int color, int depth, double alpha, double beta, const Deadline& deadline, int ply, unordered_map<uint64_t, TTEntry>& tt);

#endif
