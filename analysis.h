#ifndef RENJU_ANALYSIS_H
#define RENJU_ANALYSIS_H

#include "config.h"
#include "utils.h"
#include "board.h"

// ================= 3. 棋型分析 =================
struct AnKey {
    int color;
    uint64_t k1, k2, k3, k4;
    bool operator==(const AnKey& o) const {
        return color == o.color && k1 == o.k1 && k2 == o.k2 && k3 == o.k3 && k4 == o.k4;
    }
};
struct AnKeyHash {
    size_t operator()(const AnKey& k) const {
        size_t h = k.color;
        h ^= k.k1 + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= k.k2 + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= k.k3 + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= k.k4 + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

extern unordered_map<AnKey, tuple<bool, bool, int, int, int>, AnKeyHash> _AN_CACHE;
extern unordered_map<AnKey, double, AnKeyHash> _QE_CACHE;
extern unordered_map<uint64_t, tuple<vector<int>, vector<int>, vector<int>, vector<int>, vector<int>, vector<int>>> _SCAN_KEY_CACHE;
extern unordered_map<string, pair<double, double>> _LINE_SHAPE_CACHE;
extern unordered_map<uint64_t, pair<double, double>> _EVAL_KEY_CACHE;
extern unordered_map<uint64_t, pair<double, double>> _CENTER_KEY_CACHE;

template<typename Map>
void cap_cache(Map& c, size_t cap) {
    if (c.size() > cap) c.clear();
}

tuple<bool, bool, int, int, int> analyze_move_fb(FastBoard& fb, int x, int y, int color);

tuple<bool, bool, int, int, int> analyze_move_k(FastBoard& fb, int x, int y, int color);

tuple<vector<int>, vector<int>, vector<int>, vector<int>, vector<int>, vector<int>> _scan_line(const string& raw);

tuple<vector<int>, vector<int>, vector<int>, vector<int>, vector<int>, vector<int>> _line_scan_k(FastBoard& fb, int lid);

pair<double, double> _line_shape(const string& raw);

pair<double, double> _line_scores_k(FastBoard& fb, int lid);

pair<double, double> _center_scores_k(FastBoard& fb, int lid, int ci);

bool is_ban_move_fb(FastBoard& fb, int x, int y);

int _net_black_ban_delta(FastBoard& fb, int x, int y);

vector<pair<Pos, int>> four_moves_full(FastBoard& fb, int color);

set<Pos> _foul_trap_points(FastBoard& fb, const vector<pair<Pos, int>>* opp_fours_full = nullptr);

bool is_win_at_fb(FastBoard& fb, int x, int y, int color);

set<Pos> five_point_cells_fb(FastBoard& fb, int x, int y, int color);

bool _legal_fb(FastBoard& fb, int color, Pos p);

#endif
