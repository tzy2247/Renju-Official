#ifndef RENJU_TACTICS_H
#define RENJU_TACTICS_H

#include "config.h"
#include "utils.h"
#include "board.h"
#include "analysis.h"

// ================= 4. 攻击点扫描 =================
vector<Pos> five_moves(FastBoard& fb, int color);
vector<pair<Pos, int>> four_moves_full(FastBoard& fb, int color);
vector<Pos> four_moves(FastBoard& fb, int color);
vector<Pos> three_moves(FastBoard& fb, int color);
set<Pos> _line_vicinity_fb(FastBoard& fb, int x, int y);

// ================= 4.5 四着法质量 =================
int _four_quality(const FastBoard& fb, const Pos& p, int color);

#endif
