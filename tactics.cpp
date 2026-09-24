#include "tactics.h"

vector<Pos> five_moves(FastBoard& fb, int color) {
    set<Pos> seen;
    int k = (color == BLACK) ? 0 : 3;
    for (int lid = 0; lid < NUM_GLOBAL_LINES; ++lid) {
        if (fb.lcnt[lid] < 4) continue;
        auto scan_result = _line_scan_k(fb, lid);
        const vector<int>* idxs_ptr = nullptr;
        if (k == 0) idxs_ptr = &get<0>(scan_result);
        else idxs_ptr = &get<3>(scan_result);

        for (int e : *idxs_ptr) {
            seen.insert(GLOBAL_LINES[lid][e]);
        }
    }
    return vector<Pos>(seen.begin(), seen.end());
}

vector<pair<Pos, int>> four_moves_full(FastBoard& fb, int color) {
    set<Pos> seen;
    int k = (color == BLACK) ? 1 : 4;
    for (int lid = 0; lid < NUM_GLOBAL_LINES; ++lid) {
        if (fb.lcnt[lid] < 3) continue;
        auto scan_result = _line_scan_k(fb, lid);
        const vector<int>* idxs_ptr = nullptr;
        if (k == 1) idxs_ptr = &get<1>(scan_result);
        else idxs_ptr = &get<4>(scan_result);
        for (int e : *idxs_ptr) {
            seen.insert(GLOBAL_LINES[lid][e]);
        }
    }
    vector<pair<Pos, int>> out;
    for (auto& p : seen) {
        BoardGuard guard(fb, p.first, p.second, color);
        auto res = analyze_move_k(fb, p.first, p.second, color);
        if (!get<0>(res) && !get<1>(res) && get<2>(res) >= 1) {
            out.push_back(make_pair(p, get<2>(res)));
        }
    }
    return out;
}

vector<Pos> four_moves(FastBoard& fb, int color) {
    vector<Pos> res;
    for (auto& item : four_moves_full(fb, color)) res.push_back(item.first);
    return res;
}

vector<Pos> three_moves(FastBoard& fb, int color) {
    set<Pos> seen;
    int k = (color == BLACK) ? 2 : 5;
    for (int lid = 0; lid < NUM_GLOBAL_LINES; ++lid) {
        if (fb.lcnt[lid] < 2) continue;
        auto scan_result = _line_scan_k(fb, lid);
        const vector<int>* idxs_ptr = nullptr;
        if (k == 2) idxs_ptr = &get<2>(scan_result);
        else idxs_ptr = &get<5>(scan_result);
        for (int e : *idxs_ptr) {
            seen.insert(GLOBAL_LINES[lid][e]);
        }
    }
    vector<Pos> out;
    for (auto& p : seen) {
        BoardGuard guard(fb, p.first, p.second, color);
        auto res = analyze_move_k(fb, p.first, p.second, color);
        if (!get<0>(res) && !get<1>(res) && get<4>(res) >= 1) {
            out.push_back(p);
        }
    }
    return out;
}
set<Pos> _line_vicinity_fb(FastBoard& fb, int x, int y) {
    set<Pos> cells;
    for (auto& p : CELL_LINES[x][y]) {
        int lid = p.first;
        int ci = p.second;
        int len = (int)GLOBAL_LINES[lid].size();
        for (int k = max(0, ci - 4); k < min(len, ci + 5); ++k) {
            if (k != ci && fb.line_strings[lid][k] == B0) {
                cells.insert(GLOBAL_LINES[lid][k]);
            }
        }
    }
    return cells;
}
int _four_quality(const FastBoard& fb, const Pos& p, int color) {
    char me = (color == BLACK) ? B1 : B2;
    for (auto& q : CELL_LINES[p.first][p.second]) {
        const string& raw = fb.line_strings[q.first];
        int ci = q.second, n = (int)raw.size();
        int left = 0;
        while (ci - 1 - left >= 0 && raw[ci - 1 - left] == me) ++left;
        int right = 0;
        while (ci + 1 + right < n && raw[ci + 1 + right] == me) ++right;
        if (left + right == 3) return 1;   // 落子后恰成 4 连 (连冲四)
    }
    return 0; // 跳四或非四
}
