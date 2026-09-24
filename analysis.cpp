#include "analysis.h"
#include "tactics.h"

pair<int, int> _run_at(const string& s, int ci) {
    int i = ci, j = ci;
    while (i > 0 && s[i - 1] == B1) i--;
    while (j + 1 < (int)s.length() && s[j + 1] == B1) j++;
    return make_pair(i, j);
}

pair<set<int>, set<vector<int>>> _five_points_and_shapes(const string& s, int ci, int color) {
    int n = (int)s.length();
    set<int> pts;
    set<vector<int>> shapes;
    for (int e = max(0, ci - 4); e < min(n, ci + 5); ++e) {
        if (s[e] != B0) continue;
        int start_w = max(0, max(e, ci) - 4);
        int end_w = min(min(e, ci), n - 5);
        for (int w = start_w; w <= end_w; ++w) {
            if (!(w <= ci && ci < w + 5 && w <= e && e < w + 5)) continue;
            string win5 = s.substr(w, 5);
            int c1 = count(win5.begin(), win5.end(), B1);
            int c0 = count(win5.begin(), win5.end(), B0);
            if (c1 != 4 || c0 != 1) continue;
            if (color == BLACK) {
                if ((w > 0 && s[w - 1] == B1) || (w + 5 < n && s[w + 5] == B1)) continue;
            }
            pts.insert(e);
            vector<int> shape;
            for (int c = w; c < w + 5; ++c) {
                if (c != e) shape.push_back(c);
            }
            shapes.insert(shape);
        }
    }
    return make_pair(pts, shapes);
}

bool _is_live_three(const string& s, int ci) {
    int n = (int)s.length();
    auto ij = _run_at(s, ci);
    int i = ij.first, j = ij.second;
    if (j - i + 1 == 3) {
        if (i > 0 && j + 1 < n && s[i - 1] == B0 && s[j + 1] == B0) {
            if ((i > 1 && s[i - 2] == B0) || (j + 2 < n && s[j + 2] == B0)) return true;
        }
    }
    vector<string> pats = { "010110", "011010" };
    for (const string& pat : pats) {
        size_t st = 0;
        while (true) {
            size_t k = s.find(pat, st);
            if (k == string::npos) break;
            if ((int)k <= ci && ci < (int)k + 6) return true;
            st = k + 1;
        }
    }
    return false;
}

tuple<bool, bool, int, int, int> analyze_move_fb(FastBoard& fb, int x, int y, int color) {
    bool win = false, overline = false;
    int fp_total = 0, fs_total = 0, lt_total = 0;
    for (auto& p : CELL_LINES[x][y]) {
        int lid = p.first;
        int ci = p.second;
        string s = fb.rel(lid, color);
        auto ij = _run_at(s, ci);
        int i = ij.first, j = ij.second;
        int L = j - i + 1;
        if (L >= 6) {
            if (color == WHITE) win = true;
            else overline = true;
        }
        else if (L == 5) {
            win = true;
        }
        auto pts_shapes = _five_points_and_shapes(s, ci, color);
        auto& pts = pts_shapes.first;
        auto& shapes = pts_shapes.second;
        fp_total += (int)pts.size();
        fs_total += (int)shapes.size();
        if (_is_live_three(s, ci)) lt_total += 1;
    }
    bool foul = (color == BLACK) && (!win) && (overline || fs_total >= 2 || lt_total >= 2);
    return make_tuple(win, foul, fp_total, fs_total, lt_total);
}
unordered_map<AnKey, tuple<bool, bool, int, int, int>, AnKeyHash> _AN_CACHE;
unordered_map<AnKey, double, AnKeyHash> _QE_CACHE;
unordered_map<uint64_t, tuple<vector<int>, vector<int>, vector<int>, vector<int>, vector<int>, vector<int>>> _SCAN_KEY_CACHE;
unordered_map<string, pair<double, double>> _LINE_SHAPE_CACHE;
unordered_map<uint64_t, pair<double, double>> _EVAL_KEY_CACHE;
unordered_map<uint64_t, pair<double, double>> _CENTER_KEY_CACHE;
tuple<bool, bool, int, int, int> analyze_move_k(FastBoard& fb, int x, int y, int color) {
    auto& cl = CELL_LINES[x][y];
    AnKey key = { color, fb.lkey[cl[0].first], fb.lkey[cl[1].first], fb.lkey[cl[2].first], fb.lkey[cl[3].first] };
    auto it = _AN_CACHE.find(key);
    if (it != _AN_CACHE.end()) return it->second;
    auto ent = analyze_move_fb(fb, x, y, color);
    cap_cache(_AN_CACHE, _AN_CAP);
    _AN_CACHE[key] = ent;
    return ent;
}
tuple<vector<int>, vector<int>, vector<int>, vector<int>, vector<int>, vector<int>> _scan_line(const string& raw) {
    int n = (int)raw.length();
    vector<int> f5_b, f4_b, f3_b, f5_w, f4_w, f3_w;
    vector<pair<bool, string>> views;
    views.push_back(make_pair(true, raw));
    views.push_back(make_pair(false, swap_12(raw)));
    for (auto& v : views) {
        bool is_black = v.first;
        const string& view = v.second;
        if (count(view.begin(), view.end(), B1) >= 2) {
            for (int w = 0; w <= n - 5; ++w) {
                string win5 = view.substr(w, 5);
                int cb = count(win5.begin(), win5.end(), B1);
                if (cb < 2 || win5.find(B2) != string::npos) continue;
                int cz = 5 - cb;
                if (cb == 4 && cz == 1) {
                    if (!is_black || !((w > 0 && raw[w - 1] == B1) || (w + 5 < n && raw[w + 5] == B1))) {
                        size_t idx = win5.find(B0);
                        if (is_black) f5_b.push_back(w + (int)idx);
                        else f5_w.push_back(w + (int)idx);
                    }
                }
                else if (cb == 3 && cz == 2) {
                    for (int k = 0; k < 5; ++k) {
                        if (win5[k] == B0) {
                            if (is_black) f4_b.push_back(w + k);
                            else f4_w.push_back(w + k);
                        }
                    }
                }
                else if (cb == 2 && cz == 3) {
                    for (int k = 0; k < 5; ++k) {
                        if (win5[k] == B0) {
                            if (is_black) f3_b.push_back(w + k);
                            else f3_w.push_back(w + k);
                        }
                    }
                }
            }
        }
    }
    return make_tuple(f5_b, f4_b, f3_b, f5_w, f4_w, f3_w);
}
tuple<vector<int>, vector<int>, vector<int>, vector<int>, vector<int>, vector<int>> _line_scan_k(FastBoard& fb, int lid) {
    uint64_t key = fb.lkey[lid] ^ lid;
    auto it = _SCAN_KEY_CACHE.find(key);
    if (it != _SCAN_KEY_CACHE.end()) return it->second;
    auto ent = _scan_line(fb.line_strings[lid]);
    cap_cache(_SCAN_KEY_CACHE, _KLINE_CAP);
    _SCAN_KEY_CACHE[key] = ent;
    return ent;
}
const regex re_11111(R"(11111)");
const regex re_011110(R"(011110)");
const regex re_c4(R"(211110|011112|10111|11011|11101)");
const regex re_a3(R"(011100|001110|010110|011010)");
const regex re_m3(R"(211100|001112|210110|011012|10011|11001|10101)");
const regex re_a2(R"(001100|011000|000110|010100|001010)");
const regex re_m2(R"(211000|000112|210100|001012)");
const regex re_OVER6(R"(111111)");

int regex_count(const regex& re, const string& s) {
    auto begin = sregex_iterator(s.begin(), s.end(), re);
    auto end = sregex_iterator();
    return (int)distance(begin, end);
}
double _shape_score(const string& raw, int color) {
    string s = (color == BLACK) ? raw : swap_12(raw);
    if (color == BLACK && regex_search(s, re_OVER6)) return -5000.0;
    s = "2" + s + "2";
    double total = 0.0;
    total += 100000.0 * regex_count(re_11111, s);
    total += 12000.0 * regex_count(re_011110, s);
    total += 2500.0 * regex_count(re_c4, s);
    total += 1200.0 * regex_count(re_a3, s);
    total += 200.0 * regex_count(re_m3, s);
    total += 60.0 * regex_count(re_a2, s);
    total += 15.0 * regex_count(re_m2, s);
    return total;
}
pair<double, double> _line_shape(const string& raw) {
    auto it = _LINE_SHAPE_CACHE.find(raw);
    if (it != _LINE_SHAPE_CACHE.end()) return it->second;
    pair<double, double> ent = make_pair(_shape_score(raw, BLACK), _shape_score(raw, WHITE));
    cap_cache(_LINE_SHAPE_CACHE, _KLINE_CAP);
    _LINE_SHAPE_CACHE[raw] = ent;
    return ent;
}
pair<double, double> _line_scores_k(FastBoard& fb, int lid) {
    uint64_t key = fb.lkey[lid] ^ lid;
    auto it = _EVAL_KEY_CACHE.find(key);
    if (it != _EVAL_KEY_CACHE.end()) return it->second;
    pair<double, double> ent = _line_shape(fb.line_strings[lid]);
    cap_cache(_EVAL_KEY_CACHE, _KLINE_CAP);
    _EVAL_KEY_CACHE[key] = ent;
    return ent;
}
const uint64_t _CK_SALT[16] = {
    0ULL, 1ULL << 60, 2ULL << 60, 3ULL << 60, 4ULL << 60, 5ULL << 60, 6ULL << 60, 7ULL << 60,
    8ULL << 60, 9ULL << 60, 10ULL << 60, 11ULL << 60, 12ULL << 60, 13ULL << 60, 14ULL << 60, 15ULL << 60
};

pair<double, double> _center_scores_k(FastBoard& fb, int lid, int ci) {
    uint64_t key = fb.lkey[lid] ^ _CK_SALT[ci] ^ lid;
    auto it = _CENTER_KEY_CACHE.find(key);
    if (it != _CENTER_KEY_CACHE.end()) return it->second;
    string raw = fb.line_strings[lid];
    int n = (int)raw.length();
    double bv = 0.0, wv = 0.0;
    const double WSCORE[6] = { 0.0, 3.0, 30.0, 300.0, 4000.0, 100000.0 };
    for (int w = max(0, ci - 4); w <= min(ci, n - 5); ++w) {
        string win5 = raw.substr(w, 5);
        int cb = count(win5.begin(), win5.end(), B1);
        int cw = count(win5.begin(), win5.end(), B2);
        if (cw == 0) bv += WSCORE[cb + 1];
        if (cb == 0) wv += WSCORE[cw + 1];
    }
    pair<double, double> ent = make_pair(bv, wv);
    cap_cache(_CENTER_KEY_CACHE, _KLINE_CAP);
    _CENTER_KEY_CACHE[key] = ent;
    return ent;
}
bool is_ban_move_fb(FastBoard& fb, int x, int y) {
    if (fb.board[x][y] != EMPTY) return true;
    BoardGuard guard(fb, x, y, BLACK);
    auto res = analyze_move_k(fb, x, y, BLACK);
    return get<1>(res) && !get<0>(res);
}
int _net_black_ban_delta(FastBoard& fb, int x, int y) {
    set<Pos> cells;
    for (auto& p : CELL_LINES[x][y]) {
        int lid = p.first;
        int ci = p.second;
        int len = (int)GLOBAL_LINES[lid].size();
        for (int k = max(0, ci - 4); k < min(len, ci + 5); ++k) {
            if (k != ci) {
                Pos pt = GLOBAL_LINES[lid][k];
                if (fb.board[pt.first][pt.second] == EMPTY) cells.insert(pt);
            }
        }
    }
    if (cells.empty()) return 0;
    set<Pos> before_banned;
    for (auto& p : cells) {
        if (is_ban_move_fb(fb, p.first, p.second)) before_banned.insert(p);
    }
    BoardGuard guard(fb, x, y, BLACK);
    int delta = 0;
    for (auto& p : cells) {
        bool b_now = is_ban_move_fb(fb, p.first, p.second);
        if (b_now && before_banned.find(p) == before_banned.end()) delta += 1;
        else if (!b_now && before_banned.find(p) != before_banned.end()) delta -= 1;
    }
    return delta;
}
set<Pos> _foul_trap_points(FastBoard& fb, const vector<pair<Pos, int>>* opp_fours_full) {
    vector<pair<Pos, int>> fours = opp_fours_full ? *opp_fours_full : four_moves_full(fb, WHITE);
    set<Pos> traps;
    for (auto& item : fours) {
        Pos p = item.first;
        int fp = item.second;
        if (fp >= 2) continue;
        BoardGuard guard(fb, p.first, p.second, WHITE);
        auto fives = five_point_cells_fb(fb, p.first, p.second, WHITE);
        for (auto& q : fives) {
            if (is_ban_move_fb(fb, q.first, q.second)) {
                traps.insert(p);
                break;
            }
        }
    }
    return traps;
}
bool is_win_at_fb(FastBoard& fb, int x, int y, int color) {
    for (auto& p : CELL_LINES[x][y]) {
        int lid = p.first;
        int ci = p.second;
        string s = fb.rel(lid, color);
        auto ij = _run_at(s, ci);
        int i = ij.first, j = ij.second;
        int L = j - i + 1;
        if (L == 5 || (color == WHITE && L > 5)) return true;
    }
    return false;
}
set<Pos> five_point_cells_fb(FastBoard& fb, int x, int y, int color) {
    set<Pos> cells;
    for (auto& p : CELL_LINES[x][y]) {
        int lid = p.first;
        int ci = p.second;
        string s = fb.rel(lid, color);
        auto pts_shapes = _five_points_and_shapes(s, ci, color);
        auto& pts = pts_shapes.first;
        for (int e : pts) {
            cells.insert(GLOBAL_LINES[lid][e]);
        }
    }
    return cells;
}
bool _legal_fb(FastBoard& fb, int color, Pos p) {
    return !(color == BLACK && is_ban_move_fb(fb, p.first, p.second));
}
