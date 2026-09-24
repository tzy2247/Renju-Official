#ifndef RENJU_AI_H
#define RENJU_AI_H

#include "config.h"
#include "utils.h"
#include "board.h"
#include "analysis.h"
#include "tactics.h"
#include "eval.h"
#include "search.h"
#include "prooftree.h"
#include "negamax.h"
#include "decide.h"

// ================= 9. AI 封装 =================
struct AI {
    vector<vector<int>> board;
    int color, opp;
    FastBoard fb;
    unordered_map<uint64_t, TTEntry> vcf_tt, vct_tt, search_tt;

    AI(const vector<vector<int>>& board_2d, int c);
    void sync_board(const vector<vector<int>>& board_2d);
    optional<Pos> get_move(const vector<vector<int>>& board_2d, int c);
};

double evaluate_for_white_fb(FastBoard& fb, Pos cand);

bool is_win_at(const vector<vector<int>>& board_2d, Pos mv, int color);

vector<Pos> top_moves(const vector<vector<int>>& board_2d, int color, int n);

double evaluate_for_white(const vector<vector<int>>& board_2d, Pos cand);

optional<Pos> fallback_move(const vector<vector<int>>& board_2d, int color);

#endif
