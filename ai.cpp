#include "ai.h"

double _budget(int stones, bool opp_threat) {
    if (stones <= 8) return TIMEOUT_LIMIT * 0.35;
    if (opp_threat) return TIMEOUT_LIMIT;
    return TIMEOUT_LIMIT * 0.8;
}
AI::AI(const vector<vector<int>>& board_2d, int c) : board(board_2d), color(c), opp(1 - c), fb(board_2d) {}

void AI::sync_board(const vector<vector<int>>& board_2d) {
    Pos opp_move{ -1, -1 };
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            if (board_2d[i][j] == opp && board[i][j] != opp) {
                opp_move = { i, j };
            }
        }
    }
    board = board_2d;
    fb = FastBoard(board_2d);
    if (opp_move.first != -1) {
        mate_mode_notify(fb, opp_move);
    }
    else {
        mate_mode_reset();
    }
}

optional<Pos> AI::get_move(const vector<vector<int>>& board_2d, int c) {
    FastBoard cur_fb(board_2d);
    optional<Pos> mv;
    try {
        int opp_c = 1 - c;
        bool opp_threat = !four_moves_full(cur_fb, opp_c).empty() || !three_moves(cur_fb, opp_c).empty();
        double budget = _budget(cur_fb.stones, opp_threat);
        Deadline dl(get_time() + budget);
        mv = choose_move(cur_fb, c, dl, vcf_tt, vct_tt, search_tt);
    }
    catch (const exception& e) {
        log_debug(string("get_move exception: ") + e.what());
    }
    if (!mv.has_value()) mv = fallback_move_fb(cur_fb, c);
    if (!mv.has_value()) mv = Pos{ 0, 0 };
    return mv;
}
double evaluate_for_white_fb(FastBoard& fb, Pos cand) {
    BoardGuard guard(fb, cand.first, cand.second, BLACK);
    auto cands = fb.candidates();
    if (cands.empty()) return 0.0;
    double wb = -1e18, bb = -1e18;
    for (auto& p : cands) {
        double av_w = _attack_val(fb, p.first, p.second, WHITE);
        double av_b = _attack_val(fb, p.first, p.second, BLACK);
        if (av_w > wb) wb = av_w;
        if (av_b > bb) bb = av_b;
    }
    return wb - 0.8 * bb;
}

bool is_win_at(const vector<vector<int>>& board_2d, Pos mv, int color) {
    FastBoard fb(board_2d);
    return is_win_at_fb(fb, mv.first, mv.second, color);
}

vector<Pos> top_moves(const vector<vector<int>>& board_2d, int color, int n) {
    FastBoard fb(board_2d);
    return top_moves_fb(fb, color, n);
}

double evaluate_for_white(const vector<vector<int>>& board_2d, Pos cand) {
    FastBoard fb(board_2d);
    return evaluate_for_white_fb(fb, cand);
}

optional<Pos> fallback_move(const vector<vector<int>>& board_2d, int color) {
    FastBoard fb(board_2d);
    return fallback_move_fb(fb, color);
}
