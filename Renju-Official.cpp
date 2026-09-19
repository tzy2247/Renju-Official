#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <cmath>
#include <random>
#include <chrono>
#include <algorithm>
#include <optional>
#include <regex>
#include <sstream>
#include <cctype>
#include <tuple>
#include <memory>

using namespace std;

// ================= 0. 全局配置 =================
const bool DEBUG_MODE = true;
const bool LOCAL_TEST = true;
const double TIMEOUT_LIMIT = 40.0;

const int SIZE = 15;
const int EMPTY = -1, BLACK = 0, WHITE = 1;
const string KEEP_RUNNING = ">>>BOTZONE_REQUEST_KEEP_RUNNING<<<";

const double WIN_SCORE = 1e7;
const double MATE_MARK = WIN_SCORE - 1000;
const int VCF_DEPTH = 36;
const int VCT_DEPTH = 24;
const int VCT_ID_START = 2;
const int VCT_ID_STEP = 2;
const int BRANCH = 10;
const vector<int> ID_DEPTHS = { 2, 4, 6, 8, 10 };

const int TT_CAP = 400000;
const int _AN_CAP = 150000;
const int _QE_CAP = 150000;
const int _KLINE_CAP = 250000;

const string PHASE_OPENING = "opening_proposal";
const string PHASE_SWAP = "swap_choice";
const string PHASE_WHITE4 = "white4";
const string PHASE_BLACK5_CANDIDATES = "black5_candidates";
const string PHASE_BLACK5_SELECT = "black5_select";
const string PHASE_NORMAL = "normal_play";

using Pos = pair<int, int>;
using OpeningKey = pair<Pos, Pos>;
using OpeningVal = pair<string, string>;

map<OpeningKey, OpeningVal> OPENINGS_DB = {
    {{{7, 8}, {7, 9}}, {"寒星", "WHITE_WIN"}}, {{{7, 8}, {8, 9}}, {"溪月", "BLACK_WIN"}},
    {{{7, 8}, {9, 9}}, {"疏星", "BLACK_WIN"}}, {{{7, 8}, {8, 8}}, {"花月", "BLACK_WIN"}},
    {{{7, 8}, {9, 8}}, {"残月", "BLACK_WIN"}}, {{{7, 8}, {8, 7}}, {"雨月", "BLACK_WIN"}},
    {{{7, 8}, {9, 7}}, {"金星", "WHITE_WIN"}}, {{{7, 8}, {7, 6}}, {"松月", "WHITE_WIN"}},
    {{{7, 8}, {8, 6}}, {"丘月", "BLACK_WIN"}}, {{{7, 8}, {9, 6}}, {"新月", "BLACK_WIN"}},
    {{{7, 8}, {7, 5}}, {"瑞星", "WHITE_ADV"}}, {{{7, 8}, {8, 5}}, {"山月", "BLACK_WIN"}},
    {{{7, 8}, {9, 5}}, {"游星", "WHITE_WIN"}}, {{{8, 8}, {9, 9}}, {"长星", "BLACK_WIN"}},
    {{{8, 8}, {9, 8}}, {"峡月", "BLACK_WIN"}}, {{{8, 8}, {9, 7}}, {"恒星", "BLACK_WIN"}},
    {{{8, 8}, {9, 6}}, {"水月", "BLACK_WIN"}}, {{{8, 8}, {9, 5}}, {"流星", "BLACK_ADV"}},
    {{{8, 8}, {8, 7}}, {"云月", "WHITE_WIN"}}, {{{8, 8}, {8, 6}}, {"浦月", "BLACK_WIN"}},
    {{{8, 8}, {8, 5}}, {"岚月", "BLACK_WIN"}}, {{{8, 8}, {7, 6}}, {"银月", "BLACK_WIN"}},
    {{{8, 8}, {7, 5}}, {"明星", "BLACK_WIN"}}, {{{8, 8}, {6, 6}}, {"斜月", "WHITE_WIN"}},
    {{{8, 8}, {6, 5}}, {"名月", "WHITE_WIN"}}, {{{8, 8}, {5, 5}}, {"彗星", "WHITE_WIN"}},
};

static const vector<OpeningKey> OPENINGS_ORDER = {
    {{7, 8}, {7, 9}}, {{7, 8}, {8, 9}}, {{7, 8}, {9, 9}}, {{7, 8}, {8, 8}},
    {{7, 8}, {9, 8}}, {{7, 8}, {8, 7}}, {{7, 8}, {9, 7}}, {{7, 8}, {7, 6}},
    {{7, 8}, {8, 6}}, {{7, 8}, {9, 6}}, {{7, 8}, {7, 5}}, {{7, 8}, {8, 5}},
    {{7, 8}, {9, 5}}, {{8, 8}, {9, 9}}, {{8, 8}, {9, 8}}, {{8, 8}, {9, 7}},
    {{8, 8}, {9, 6}}, {{8, 8}, {9, 5}}, {{8, 8}, {8, 7}}, {{8, 8}, {8, 6}},
    {{8, 8}, {8, 5}}, {{8, 8}, {7, 6}}, {{8, 8}, {7, 5}}, {{8, 8}, {6, 6}},
    {{8, 8}, {6, 5}}, {{8, 8}, {5, 5}},
};

// ================= 1. 基础工具 =================
struct TimeoutException : public exception {};

double get_time() {
    return chrono::duration<double>(chrono::steady_clock::now().time_since_epoch()).count();
}

struct Deadline {
    double t;
    Deadline(double t) : t(t) {}
    double left() const { return t - get_time(); }
    void check() const {
        if (get_time() > t) throw TimeoutException();
    }
};

void log_debug(const string& msg) {
    if (DEBUG_MODE) cerr << "[DEBUG] " << msg << endl;
}

mt19937_64 rng(1337);
uint64_t ZOBRIST[3][SIZE][SIZE];

vector<vector<Pos>> GLOBAL_LINES;
vector<vector<vector<Pos>>> CELL_LINES;
uint64_t LINE_Z[100][20][2];
int NUM_GLOBAL_LINES = 0;

const char B0 = '0', B1 = '1', B2 = '2';

string swap_12(const string& s) {
    string res = s;
    for (char& c : res) {
        if (c == '1') c = '2';
        else if (c == '2') c = '1';
    }
    return res;
}

void init_globals() {
    for (int c = 0; c < 3; ++c)
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                ZOBRIST[c][i][j] = rng();

    for (int i = 0; i < SIZE; ++i) {
        vector<Pos> line;
        for (int j = 0; j < SIZE; ++j) line.push_back(make_pair(i, j));
        GLOBAL_LINES.push_back(line);
    }
    for (int j = 0; j < SIZE; ++j) {
        vector<Pos> line;
        for (int i = 0; i < SIZE; ++i) line.push_back(make_pair(i, j));
        GLOBAL_LINES.push_back(line);
    }
    for (int s = 0; s < 2 * SIZE - 1; ++s) {
        vector<Pos> line;
        for (int r = 0; r < SIZE; ++r) {
            int c = s - r;
            if (c >= 0 && c < SIZE) line.push_back(make_pair(r, c));
        }
        if (!line.empty()) GLOBAL_LINES.push_back(line);
    }
    for (int d = -(SIZE - 1); d < SIZE; ++d) {
        vector<Pos> line;
        for (int r = 0; r < SIZE; ++r) {
            int c = r - d;
            if (c >= 0 && c < SIZE) line.push_back(make_pair(r, c));
        }
        if (!line.empty()) GLOBAL_LINES.push_back(line);
    }

    NUM_GLOBAL_LINES = (int)GLOBAL_LINES.size();
    CELL_LINES.assign(SIZE, vector<vector<Pos>>(SIZE));
    for (int lid = 0; lid < NUM_GLOBAL_LINES; ++lid) {
        for (size_t idx = 0; idx < GLOBAL_LINES[lid].size(); ++idx) {
            int x = GLOBAL_LINES[lid][idx].first;
            int y = GLOBAL_LINES[lid][idx].second;
            CELL_LINES[x][y].push_back(make_pair(lid, (int)idx));
        }
    }

    for (int lid = 0; lid < NUM_GLOBAL_LINES; ++lid) {
        for (size_t idx = 0; idx < GLOBAL_LINES[lid].size(); ++idx) {
            LINE_Z[lid][idx][0] = rng();
            LINE_Z[lid][idx][1] = rng();
        }
    }
}

// ================= 2. 预计算线表 & 棋盘 =================
struct FastBoard {
    int board[SIZE][SIZE];
    vector<string> line_strings;
    uint64_t hash;
    int near[SIZE][SIZE];
    int stones;
    int lcnt[100];
    uint64_t lkey[100];

    FastBoard() {
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                board[i][j] = EMPTY;
        for (int i = 0; i < NUM_GLOBAL_LINES; ++i)
            line_strings.push_back(string(GLOBAL_LINES[i].size(), B0));
        hash = 0;
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                near[i][j] = 0;
        stones = 0;
        for (int i = 0; i < NUM_GLOBAL_LINES; ++i) {
            lcnt[i] = 0;
            lkey[i] = 0;
        }
    }

    FastBoard(const vector<vector<int>>& board_2d) : FastBoard() {
        for (int i = 0; i < SIZE; ++i) {
            for (int j = 0; j < SIZE; ++j) {
                if (board_2d[i][j] != EMPTY) {
                    place(i, j, board_2d[i][j]);
                }
            }
        }
    }

    void place(int x, int y, int color) {
        char ch = (color == BLACK) ? B1 : B2;
        board[x][y] = color;
        hash ^= ZOBRIST[color + 1][x][y];
        stones += 1;
        for (auto& p : CELL_LINES[x][y]) {
            int lid = p.first;
            int idx = p.second;
            line_strings[lid][idx] = ch;
            lcnt[lid] += 1;
            lkey[lid] ^= LINE_Z[lid][idx][color];
        }
        for (int di = -2; di <= 2; ++di) {
            for (int dj = -2; dj <= 2; ++dj) {
                if (di == 0 && dj == 0) continue;
                int nx = x + di, ny = y + dj;
                if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE) {
                    near[nx][ny] += 1;
                }
            }
        }
    }

    void undo(int x, int y, int color) {
        board[x][y] = EMPTY;
        hash ^= ZOBRIST[color + 1][x][y];
        stones -= 1;
        for (auto& p : CELL_LINES[x][y]) {
            int lid = p.first;
            int idx = p.second;
            line_strings[lid][idx] = B0;
            lcnt[lid] -= 1;
            lkey[lid] ^= LINE_Z[lid][idx][color];
        }
        for (int di = -2; di <= 2; ++di) {
            for (int dj = -2; dj <= 2; ++dj) {
                if (di == 0 && dj == 0) continue;
                int nx = x + di, ny = y + dj;
                if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE) {
                    near[nx][ny] -= 1;
                }
            }
        }
    }

    string rel(int lid, int color) const {
        if (color == WHITE) return swap_12(line_strings[lid]);
        return line_strings[lid];
    }

    vector<Pos> candidates() const {
        vector<Pos> res;
        for (int i = 0; i < SIZE; ++i) {
            for (int j = 0; j < SIZE; ++j) {
                if (board[i][j] == EMPTY && near[i][j] > 0) {
                    res.push_back(make_pair(i, j));
                }
            }
        }
        return res;
    }
};

struct BoardGuard {
    FastBoard& fb;
    int x, y, color;
    BoardGuard(FastBoard& b, int x, int y, int c) : fb(b), x(x), y(y), color(c) {
        fb.place(x, y, color);
    }
    ~BoardGuard() {
        fb.undo(x, y, color);
    }
};

// ================= 3. 棋型分析 =================
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

// 【修复】跳三判定：必须验证"补空隙后确实能形成活四（黑棋还要验证不构成长连）"
bool _is_live_three(const string& s, int ci, int color) {
    int n = (int)s.length();
    auto ij = _run_at(s, ci);
    int i = ij.first, j = ij.second;

    // ---- 直三 ----
    if (j - i + 1 == 3) {
        if (i > 0 && j + 1 < n && s[i - 1] == B0 && s[j + 1] == B0) {
            if (color != BLACK) {
                if ((i > 1 && s[i - 2] == B0) || (j + 2 < n && s[j + 2] == B0)) return true;
            }
            else {
                // 左延：落 i-1 成 [i-1, j]，成五点 i-2 与 j+1
                bool left_ok = (i > 1 && s[i - 2] == B0)
                    && (i - 3 < 0 || s[i - 3] != B1)
                    && (j + 2 >= n || s[j + 2] != B1);
                // 右延：落 j+1 成 [i, j+1]，成五点 i-1 与 j+2
                bool right_ok = (j + 2 < n && s[j + 2] == B0)
                    && (j + 3 >= n || s[j + 3] != B1)
                    && (i - 2 < 0 || s[i - 2] != B1);
                if (left_ok || right_ok) return true;
            }
        }
    }

    // ---- 跳三 ----
    // 模式: k..k+5 = "010110" 或 "011010"
    // 补空隙后得到 [k+1, k+4] 的连续四，成五点是 k 和 k+5。
    // 判定要点:
    //   1) ci 必须真的落在模式窗口内（保证刚下的这一子构成该模式）
    //   2) 补空隙后形成的四两侧都必须是空点
    //   3) 黑棋还要确保在 k 或 k+5 落子不会形成六连
    vector<string> pats = { "010110", "011010" };
    for (const string& pat : pats) {
        size_t st = 0;
        while (true) {
            size_t k = s.find(pat, st);
            if (k == string::npos) break;
            if ((int)k <= ci && ci < (int)k + 6) {
                // 补空隙后 [k+1, k+4] 为四连；成五点 k 与 k+5 已由模式保证为空
                int L = (int)k + 1;
                int R = (int)k + 4;

                // 两端成五点必须为空（模式已保证，这里再稳妥确认）
                bool gap_fill_ok =
                    (L - 1 >= 0 && s[L - 1] == B0) &&
                    (R + 1 < n && s[R + 1] == B0);
                if (!gap_fill_ok) { st = k + 1; continue; }

                if (color != BLACK) {
                    // 白棋无禁手，宽松接受
                    return true;
                }

                // 黑棋：分别检查"在 k 落子成五"和"在 k+5 落子成五"是否构成长连
                // 落 k 后五连为 [k, k+4]，需要 (k-1) 不是黑子 且 (k+5) 不是黑子
                bool left_five_clean =
                    (k == 0 || s[(int)k - 1] != B1) &&
                    (s[(int)k + 5] != B1);

                // 落 k+5 后五连为 [k+1, k+5]，需要 k 不是黑子 且 (k+6) 不是黑子
                bool right_five_clean =
                    (s[(int)k] != B1) &&
                    ((int)k + 6 >= n || s[(int)k + 6] != B1);

                if (left_five_clean && right_five_clean) return true;
            }
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
        if (_is_live_three(s, ci, color)) lt_total += 1;
    }
    bool foul = (color == BLACK) && (!win) && (overline || fs_total >= 2 || lt_total >= 2);
    return make_tuple(win, foul, fp_total, fs_total, lt_total);
}

// ================= 3.5 线内容级缓存 =================
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

unordered_map<AnKey, tuple<bool, bool, int, int, int>, AnKeyHash> _AN_CACHE;
unordered_map<AnKey, double, AnKeyHash> _QE_CACHE;
unordered_map<uint64_t, tuple<vector<int>, vector<int>, vector<int>, vector<int>, vector<int>, vector<int>>> _SCAN_KEY_CACHE;
unordered_map<string, pair<double, double>> _LINE_SHAPE_CACHE;
unordered_map<uint64_t, pair<double, double>> _EVAL_KEY_CACHE;
unordered_map<uint64_t, pair<double, double>> _CENTER_KEY_CACHE;

template<typename Map>
void cap_cache(Map& c, size_t cap) {
    if (c.size() > cap) c.clear();
}

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

set<Pos> five_point_cells_fb(FastBoard& fb, int x, int y, int color);
vector<pair<Pos, int>> four_moves_full(FastBoard& fb, int color);

set<Pos> _foul_trap_points(FastBoard& fb, const vector<pair<Pos, int>>* opp_fours_full = nullptr) {
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

// ================= 4. 攻击点扫描 =================
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

// ================= 5. 评估 =================
double _attack_val(FastBoard& fb, int x, int y, int color) {
    double v = 0.0;
    for (auto& p : CELL_LINES[x][y]) {
        auto scores = _center_scores_k(fb, p.first, p.second);
        double bv = scores.first, wv = scores.second;
        v += (color == BLACK) ? bv : wv;
    }
    return v;
}

double quick_eval_fb(FastBoard& fb, int x, int y, int color) {
    auto& cl = CELL_LINES[x][y];
    AnKey key = { color, fb.lkey[cl[0].first], fb.lkey[cl[1].first], fb.lkey[cl[2].first], fb.lkey[cl[3].first] };
    auto it = _QE_CACHE.find(key);
    if (it != _QE_CACHE.end()) return it->second;
    double v = 0.0;
    for (auto& p : cl) {
        auto scores = _center_scores_k(fb, p.first, p.second);
        double bv = scores.first, wv = scores.second;
        if (color == BLACK) v += bv + 0.85 * wv;
        else v += wv + 0.85 * bv;
    }
    cap_cache(_QE_CACHE, _QE_CAP);
    _QE_CACHE[key] = v;
    return v;
}

double evaluate_fb(FastBoard& fb, int tomove) {
    double me = 0.0, op = 0.0;
    for (int lid = 0; lid < NUM_GLOBAL_LINES; ++lid) {
        if (fb.lcnt[lid] == 0) continue;
        auto scores = _line_scores_k(fb, lid);
        double bs = scores.first, ws = scores.second;
        if (bs > 0) {
            if (tomove == BLACK) me += bs;
            else op += bs;
        }
        if (ws > 0) {
            if (tomove == WHITE) me += ws;
            else op += ws;
        }
    }
    return me - 0.9 * op + 50.0;
}

optional<Pos> _best_by_eval_fb(FastBoard& fb, const vector<Pos>& moves, int color) {
    if (moves.empty()) return nullopt;
    optional<Pos> best = moves[0];
    double max_v = quick_eval_fb(fb, moves[0].first, moves[0].second, color);
    for (size_t i = 1; i < moves.size(); ++i) {
        double v = quick_eval_fb(fb, moves[i].first, moves[i].second, color);
        if (v > max_v) {
            max_v = v;
            best = moves[i];
        }
    }
    return best;
}

optional<Pos> fallback_move_fb(FastBoard& fb, int color) {
    auto cands = fb.candidates();
    if (cands.empty()) {
        if (fb.stones == 0) return Pos{ SIZE / 2, SIZE / 2 };
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                if (fb.board[i][j] == EMPTY) cands.push_back(make_pair(i, j));
        if (cands.empty()) return nullopt;
    }
    vector<Pos> legal;
    for (auto& p : cands) {
        if (_legal_fb(fb, color, p)) legal.push_back(p);
    }
    auto& pool = legal.empty() ? cands : legal;
    optional<Pos> best = pool[0];
    double max_v = quick_eval_fb(fb, pool[0].first, pool[0].second, color);
    for (size_t i = 1; i < pool.size(); ++i) {
        double v = quick_eval_fb(fb, pool[i].first, pool[i].second, color);
        if (v > max_v) {
            max_v = v;
            best = pool[i];
        }
    }
    return best;
}

vector<Pos> top_moves_fb(FastBoard& fb, int color, int n) {
    auto cands = fb.candidates();
    if (cands.empty()) {
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                if (fb.board[i][j] == EMPTY) cands.push_back(make_pair(i, j));
    }
    vector<Pos> legal;
    for (auto& p : cands) {
        if (_legal_fb(fb, color, p)) legal.push_back(p);
    }
    auto& pool = legal.empty() ? cands : legal;
    sort(pool.begin(), pool.end(), [&](const Pos& a, const Pos& b) {
        return quick_eval_fb(fb, a.first, a.second, color) > quick_eval_fb(fb, b.first, b.second, color);
        });
    if ((int)pool.size() > n) pool.resize(n);
    return pool;
}

// ================= 6. VCF/VCT 证明树 =================
struct TTEntry {
    int depth;
    int flag;
    double val;
    optional<Pos> mv;
};
unordered_map<uint64_t, TTEntry> tt_vcf, tt_vct, tt_search;

void _tt_store(unordered_map<uint64_t, TTEntry>& tt, uint64_t key, int depth, double val, optional<Pos> mv, int flag = 0) {
    if ((int)tt.size() > TT_CAP) tt.clear();
    tt[key] = { depth, flag, val, mv };
}

void _tt_store_fail(unordered_map<uint64_t, TTEntry>& tt, uint64_t key, int depth) {
    auto it = tt.find(key);
    if (it != tt.end() && it->second.mv.has_value()) return;
    if (it != tt.end() && it->second.depth >= depth) return;
    if ((int)tt.size() > TT_CAP) tt.clear();
    tt[key] = { depth, 2, 0, nullopt };
}

pair<bool, optional<TTEntry>> _tt_get(const unordered_map<uint64_t, TTEntry>& tt, uint64_t key, int depth) {
    auto it = tt.find(key);
    if (it == tt.end()) return make_pair(false, nullopt);
    if (it->second.mv.has_value()) return make_pair(depth >= it->second.depth, it->second);
    return make_pair(depth <= it->second.depth, it->second);
}

vector<pair<Pos, int>> _threat_moves_scored(FastBoard& fb, int color) {
    set<Pos> cands;
    for (auto& item : four_moves_full(fb, color)) cands.insert(item.first);
    for (auto& p : three_moves(fb, color)) cands.insert(p);
    vector<pair<Pos, int>> out;
    for (auto& p : cands) {
        BoardGuard guard(fb, p.first, p.second, color);
        auto res = analyze_move_k(fb, p.first, p.second, color);
        if (get<0>(res) || get<1>(res)) continue;
        int sc = 0;
        if (get<2>(res) >= 2) sc = 100;
        else if (get<2>(res) >= 1 && get<4>(res) >= 1) sc = 50;
        else if (get<4>(res) >= 2) sc = 20;
        if (sc) out.push_back(make_pair(p, sc));
    }
    sort(out.begin(), out.end(), [](const pair<Pos, int>& a, const pair<Pos, int>& b) {
        return a.second > b.second;
        });
    return out;
}

vector<Pos> _threat_moves(FastBoard& fb, int color) {
    vector<Pos> res;
    for (auto& item : _threat_moves_scored(fb, color)) res.push_back(item.first);
    return res;
}

vector<Pos> _defense_candidates(FastBoard& fb, int color, const set<Pos>& block_pts) {
    set<Pos> cands = block_pts;
    for (auto& item : four_moves_full(fb, color)) cands.insert(item.first);
    for (auto& item : _threat_moves_scored(fb, color)) {
        if (item.second >= 20) cands.insert(item.first);
    }
    vector<Pos> res;
    for (auto& p : cands) {
        if (_legal_fb(fb, color, p)) res.push_back(p);
    }
    return res;
}

optional<vector<Pos>> search_vcf(FastBoard& fb, int color, int depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt);
optional<vector<Pos>> search_vct(FastBoard& fb, int color, int depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt, set<pair<uint64_t, int>>* path_ptr);

optional<vector<Pos>> search_vcf(FastBoard& fb, int color, int depth,
    const Deadline& deadline,
    unordered_map<uint64_t, TTEntry>& tt) {
    deadline.check();
    uint64_t key = fb.hash ^ ((uint64_t)color << 50) ^ 0x12345678ULL;
    auto tt_result = _tt_get(tt, key, depth);
    if (tt_result.first && tt_result.second.has_value()
        && tt_result.second->mv.has_value()) {
        return vector<Pos>{ tt_result.second->mv.value() };
    }

    auto fives = five_moves(fb, color);
    if (!fives.empty()) {
        _tt_store(tt, key, depth, 0, fives[0], 0);
        return vector<Pos>{ fives[0] };
    }
    if (depth <= 0) {
        _tt_store_fail(tt, key, depth);
        return nullopt;
    }

    int opp = 1 - color;
    auto fours = four_moves_full(fb, color);
    sort(fours.begin(), fours.end(),
        [](const pair<Pos, int>& a, const pair<Pos, int>& b) {
            return a.second > b.second;
        });

    for (auto& item : fours) {
        Pos atk = item.first;
        deadline.check();
        BoardGuard guard(fb, atk.first, atk.second, color);

        // 攻方落子后，若守方立刻能成五，则本攻击着法失败
        if (!five_moves(fb, opp).empty()) continue;

        auto defs = five_point_cells_fb(fb, atk.first, atk.second, color);
        if (defs.empty()) continue;

        bool all_win = true;
        for (auto& df : defs) {
            deadline.check();
            BoardGuard guard2(fb, df.first, df.second, opp);

            // ========== 关键修复：守方直接获胜检查（包含白方） ==========
            if (is_win_at_fb(fb, df.first, df.second, opp)) {
                all_win = false;
                break;
            }

            // 黑方守棋：检查禁手
            if (opp == BLACK) {
                auto res = analyze_move_k(fb, df.first, df.second, opp);
                if (get<0>(res)) {          // 黑直接五连 → 攻方失败
                    all_win = false;
                    break;
                }
                if (get<1>(res)) {          // 黑禁手 → 视为攻方胜，跳过此应手
                    continue;
                }
            }

            auto sub = search_vcf(fb, color, depth - 1, deadline, tt);
            if (!sub.has_value()) {
                all_win = false;
                break;
            }
        }
        if (all_win) {
            _tt_store(tt, key, depth, 0, atk, 0);
            return vector<Pos>{ atk };
        }
    }
    _tt_store_fail(tt, key, depth);
    return nullopt;
}
// ================= 6. VCT/VCF 证明树（修复版）=================
static vector<Pos> _vct_defense_moves(FastBoard& fb, int attacker, int defender) {
    set<Pos> defs;

    // (a) 攻方下一手即可成五的点 —— 守方必须占领
    for (auto& p : five_moves(fb, attacker)) defs.insert(p);

    // (b) 攻方能一步形成"四"的所有点（含单四）
    //     原实现只保留 fp>=2 的点，导致攻方做活三后其单四延伸点未被检验
    for (auto& item : four_moves_full(fb, attacker)) defs.insert(item.first);

    // (c) 守方自己的冲四点（反四）
    for (auto& item : four_moves_full(fb, defender)) defs.insert(item.first);

    // (d) 守方自己的活三点（反活三）
    for (auto& p : three_moves(fb, defender)) defs.insert(p);

    // (e) 攻方为黑时，守方（白）的禁手陷阱点
    if (attacker == BLACK) {
        auto traps = _foul_trap_points(fb);
        for (auto& p : traps) defs.insert(p);
    }

    // 合法性过滤
    vector<Pos> legal;
    legal.reserve(defs.size());
    for (auto& p : defs) {
        if (fb.board[p.first][p.second] != EMPTY) continue;
        if (defender == BLACK && is_ban_move_fb(fb, p.first, p.second)) continue;
        legal.push_back(p);
    }
    return legal;
}

optional<vector<Pos>> search_vct(FastBoard& fb, int color, int depth,
    const Deadline& deadline,
    unordered_map<uint64_t, TTEntry>& tt,
    set<pair<uint64_t, int>>* path_ptr = nullptr) {
    deadline.check();
    set<pair<uint64_t, int>> local_path;
    set<pair<uint64_t, int>>& path = path_ptr ? *path_ptr : local_path;
    uint64_t key_h = fb.hash ^ ((uint64_t)color << 50);
    if (path.count(make_pair(key_h, color))) return nullopt;

    uint64_t key = key_h ^ 0x87654321ULL;
    auto tt_result = _tt_get(tt, key, depth);
    if (tt_result.first && tt_result.second.has_value()
        && tt_result.second->mv.has_value()) {
        return vector<Pos>{ tt_result.second->mv.value() };
    }

    auto fives = five_moves(fb, color);
    if (!fives.empty()) {
        _tt_store(tt, key, depth, 0, fives[0], 0);
        return vector<Pos>{ fives[0] };
    }
    if (depth <= 0) {
        _tt_store_fail(tt, key, depth);
        return nullopt;
    }
    int opp = 1 - color;

    // 攻击候选：先冲四，后威胁着法
    auto fours_sorted = four_moves_full(fb, color);
    sort(fours_sorted.begin(), fours_sorted.end(),
        [](const pair<Pos, int>& a, const pair<Pos, int>& b) {
            return a.second > b.second;
        });

    vector<Pos> atks;
    set<Pos> seen_atk;
    for (auto& item : fours_sorted) {
        if (seen_atk.insert(item.first).second) atks.push_back(item.first);
    }
    for (auto& p : _threat_moves(fb, color)) {
        if (seen_atk.insert(p).second) atks.push_back(p);
    }
    if (atks.empty()) {
        _tt_store_fail(tt, key, depth);
        return nullopt;
    }

    path.insert(make_pair(key_h, color));
    optional<vector<Pos>> result = nullopt;
    try {
        for (auto& atk : atks) {
            deadline.check();
            BoardGuard guard(fb, atk.first, atk.second, color);

            // 攻方落子后，守方若立刻能成五，则本攻击候选无效
            if (!five_moves(fb, opp).empty()) continue;

            // ===== 动态生成守方全部必须考虑的应手 =====
            vector<Pos> legal_defs = _vct_defense_moves(fb, color, opp);

            if (legal_defs.empty()) {
                // 守方无任何合法应手 → 攻方胜
                _tt_store(tt, key, depth, 0, atk, 0);
                result = vector<Pos>{ atk };
                break;
            }

            bool all_win = true;
            for (auto& df : legal_defs) {
                deadline.check();
                BoardGuard guard2(fb, df.first, df.second, opp);

                // ===== 关键修复：守方直接获胜检查（含白方） =====
                if (is_win_at_fb(fb, df.first, df.second, opp)) {
                    all_win = false;
                    break;
                }

                // 守方为黑：再检查禁手
                if (opp == BLACK) {
                    auto res_d = analyze_move_k(fb, df.first, df.second, opp);
                    if (get<0>(res_d)) {         // 黑五连 → 攻方失败
                        all_win = false;
                        break;
                    }
                    if (get<1>(res_d)) {         // 黑禁手 → 跳过此应手
                        continue;
                    }
                }

                auto sub = search_vct(fb, color, depth - 1, deadline, tt, &path);
                if (!sub.has_value()) {
                    all_win = false;
                    break;
                }
            }

            if (all_win) {
                _tt_store(tt, key, depth, 0, atk, 0);
                result = vector<Pos>{ atk };
                break;
            }
        }
        if (!result.has_value()) _tt_store_fail(tt, key, depth);
    }
    catch (...) {
        path.erase(make_pair(key_h, color));
        throw;
    }
    path.erase(make_pair(key_h, color));
    return result;
}

optional<vector<Pos>> search_vct_id(FastBoard& fb, int color, int max_depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt) {
    for (int d = VCT_ID_START; d <= max_depth; d += VCT_ID_STEP) {
        deadline.check();
        auto r = search_vct(fb, color, d, deadline, tt);
        if (r.has_value()) return r;
        if (deadline.left() < 1.5) return nullopt;
    }
    return nullopt;
}

bool vcf_disproved(FastBoard& fb, int color, int max_depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt) {
    try {
        auto result = search_vcf(fb, color, max_depth, deadline, tt);
        return !result.has_value();
    }
    catch (const TimeoutException&) {
        return false;
    }
}

bool vct_disproved(FastBoard& fb, int color, int max_depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt) {
    try {
        for (int d = VCT_ID_START; d <= max_depth; d += VCT_ID_STEP) {
            deadline.check();
            if (search_vct(fb, color, d, deadline, tt).has_value()) return false;
        }
        return true;
    }
    catch (const TimeoutException&) {
        return false;
    }
}

// ================= 7. Negamax =================
double _score_to_tt(double sc, int ply) {
    if (sc > MATE_MARK) return sc + ply;
    if (sc < -MATE_MARK) return sc - ply;
    return sc;
}

double _score_from_tt(double sc, int ply) {
    if (sc > MATE_MARK) return sc - ply;
    if (sc < -MATE_MARK) return sc + ply;
    return sc;
}

struct PairHash {
    size_t operator()(const pair<int, Pos>& p) const {
        return hash<int>()(p.first) ^ (hash<int>()(p.second.first) << 1) ^ (hash<int>()(p.second.second) << 2);
    }
};
unordered_map<pair<int, Pos>, int, PairHash> HIST;
unordered_map<int, Pos> KILLER;

pair<double, optional<Pos>> negamax(FastBoard& fb, int color, int depth, double alpha, double beta, const Deadline& deadline, int ply, unordered_map<uint64_t, TTEntry>& tt) {
    deadline.check();
    uint64_t key = fb.hash ^ ((uint64_t)color << 50);
    optional<TTEntry> ent_opt = nullopt;
    auto it = tt.find(key);
    if (it != tt.end()) ent_opt = it->second;

    optional<Pos> tt_move = nullopt;
    if (ent_opt.has_value()) {
        tt_move = ent_opt->mv;
        if (ent_opt->depth >= depth) {
            double val = _score_from_tt(ent_opt->val, ply);
            if (ent_opt->flag == 0) return make_pair(val, tt_move);
            if (ent_opt->flag == 1 && val >= beta) return make_pair(val, tt_move);
            if (ent_opt->flag == 2 && val <= alpha) return make_pair(val, tt_move);
        }
    }
    if (depth <= 0) return make_pair(evaluate_fb(fb, color), nullopt);

    auto own_fives = five_moves(fb, color);
    if (!own_fives.empty()) return make_pair(WIN_SCORE - ply, own_fives[0]);

    int opp = 1 - color;
    auto opp_fives = five_moves(fb, opp);
    auto cands = fb.candidates();
    if (cands.empty()) return make_pair(0.0, nullopt);
    if (!opp_fives.empty()) cands = opp_fives;

    auto hget = [&](const pair<int, Pos>& k) {
        auto hit = HIST.find(k);
        return hit != HIST.end() ? hit->second : 0;
        };
    auto killer_it = KILLER.find(ply);
    optional<Pos> killer = nullopt;
    if (killer_it != KILLER.end()) killer = killer_it->second;

    auto _sk = [&](const Pos& a, const Pos& b) {
        double va = quick_eval_fb(fb, a.first, a.second, color) + 0.5 * hget(make_pair(color, a));
        double vb = quick_eval_fb(fb, b.first, b.second, color) + 0.5 * hget(make_pair(color, b));
        if (killer.has_value()) {
            if (a == killer.value()) va += 400.0;
            if (b == killer.value()) vb += 400.0;
        }
        return va > vb;
        };

    sort(cands.begin(), cands.end(), _sk);
    if (tt_move.has_value() && fb.board[tt_move->first][tt_move->second] == EMPTY) {
        auto it_find = find(cands.begin(), cands.end(), tt_move.value());
        if (it_find != cands.end()) {
            cands.erase(it_find);
            cands.insert(cands.begin(), tt_move.value());
        }
    }

    vector<Pos> pool;
    if (color == BLACK) {
        for (auto& p : cands) {
            if (!is_ban_move_fb(fb, p.first, p.second)) {
                pool.push_back(p);
                if ((int)pool.size() >= BRANCH) break;
            }
        }
        if (pool.empty()) return make_pair(-WIN_SCORE + ply, nullopt);
    }
    else {
        int limit = min((int)cands.size(), BRANCH);
        pool = vector<Pos>(cands.begin(), cands.begin() + limit);
    }

    double alpha0 = alpha;
    double best = -1e18;
    optional<Pos> best_mv = pool[0];

    for (auto& mv : pool) {
        deadline.check();
        BoardGuard guard(fb, mv.first, mv.second, color);
        double sc;
        if (is_win_at_fb(fb, mv.first, mv.second, color)) {
            sc = WIN_SCORE - ply;
        }
        else {
            auto res = analyze_move_k(fb, mv.first, mv.second, color);
            int nd = (get<2>(res) >= 1) ? depth : depth - 1;
            if (ply > 40) nd = depth - 1;
            auto sub = negamax(fb, 1 - color, nd, -beta, -alpha, deadline, ply + 1, tt);
            sc = -sub.first;
        }
        if (sc > best) {
            best = sc;
            best_mv = mv;
        }
        if (sc > alpha) alpha = sc;
        if (alpha >= beta) {
            pair<int, Pos> hk = make_pair(color, mv);
            HIST[hk] = hget(hk) + depth * depth;
            KILLER[ply] = mv;
            if ((int)HIST.size() > 200000) {
                for (auto& kv : HIST) kv.second /= 2;
            }
            break;
        }
    }

    int flg = 0;
    if (best >= beta) flg = 1;
    else if (best <= alpha0) flg = 2;

    if ((int)tt.size() > TT_CAP) tt.clear();
    tt[key] = { depth, flg, _score_to_tt(best, ply), best_mv };
    return make_pair(best, best_mv);
}

// ================= 8. 主决策 =================
bool _has_double_points_fb(FastBoard& fb, int color, const Deadline& deadline) {
    for (auto& item : four_moves_full(fb, color)) {
        deadline.check();
        if (item.second >= 2) return true;
    }
    return false;
}

optional<Pos> choose_move(FastBoard& fb, int color, const Deadline& deadline,
    unordered_map<uint64_t, TTEntry>& vcf_tt,
    unordered_map<uint64_t, TTEntry>& vct_tt,
    unordered_map<uint64_t, TTEntry>& search_tt) {
    int opp = 1 - color;

    auto fives = five_moves(fb, color);
    if (!fives.empty()) return _best_by_eval_fb(fb, fives, color);

    auto opp_fives = five_moves(fb, opp);
    if (!opp_fives.empty()) {
        vector<Pos> blocks;
        for (auto& p : opp_fives) if (_legal_fb(fb, color, p)) blocks.push_back(p);
        if (!blocks.empty()) return _best_by_eval_fb(fb, blocks, color);
        return fallback_move_fb(fb, color);
    }

    auto my_fours = four_moves_full(fb, color);
    vector<Pos> my_dbl;
    for (auto& item : my_fours) if (item.second >= 2) my_dbl.push_back(item.first);
    if (!my_dbl.empty()) return _best_by_eval_fb(fb, my_dbl, color);

    auto sub_deadline = [&](double frac) {
        return Deadline(min(get_time() + max(deadline.left() * frac, 0.5), deadline.t));
        };

    try {
        auto path = search_vcf(fb, color, VCF_DEPTH, sub_deadline(0.4), vcf_tt);
        if (path.has_value()) return (*path)[0];
    }
    catch (const TimeoutException&) {}

    auto opp_fours_full = four_moves_full(fb, opp);
    vector<Pos> opp_dbl;
    for (auto& item : opp_fours_full) if (item.second >= 2) opp_dbl.push_back(item.first);
    if (!opp_dbl.empty()) {
        set<Pos> opp_dbl_set(opp_dbl.begin(), opp_dbl.end());
        auto resp = _defense_candidates(fb, color, opp_dbl_set);
        sort(resp.begin(), resp.end(), [&](const Pos& a, const Pos& b) {
            return quick_eval_fb(fb, a.first, a.second, color) > quick_eval_fb(fb, b.first, b.second, color);
            });
        if ((int)resp.size() > 10) resp.resize(10);
        optional<Pos> best_mv;
        double best_v = -1e18;
        for (auto& mv : resp) {
            bool bad;
            {
                BoardGuard guard(fb, mv.first, mv.second, color);
                bad = !five_moves(fb, opp).empty() || _has_double_points_fb(fb, opp, deadline);
            }
            if (!bad) {
                double v = quick_eval_fb(fb, mv.first, mv.second, color);
                if (v > best_v) { best_v = v; best_mv = mv; }
            }
        }
        if (best_mv.has_value()) return best_mv;
        if (!resp.empty()) return resp[0];
    }

    if (color == BLACK) {
        auto traps = _foul_trap_points(fb, &opp_fours_full);
        if (!traps.empty()) {
            auto resp = _defense_candidates(fb, color, traps);
            sort(resp.begin(), resp.end(), [&](const Pos& a, const Pos& b) {
                bool a_in = traps.count(a) > 0;
                bool b_in = traps.count(b) > 0;
                if (a_in != b_in) return a_in > b_in;
                int da = _net_black_ban_delta(fb, a.first, a.second);
                int db = _net_black_ban_delta(fb, b.first, b.second);
                if (da != db) return da < db;
                return quick_eval_fb(fb, a.first, a.second, color) > quick_eval_fb(fb, b.first, b.second, color);
                });
            if ((int)resp.size() > 8) resp.resize(8);
            double v_end = get_time() + deadline.left() * 0.3;
            for (auto& mv : resp) {
                if (get_time() >= v_end || deadline.left() < 1.5) break;
                BoardGuard guard2(fb, mv.first, mv.second, color);
                if (!five_moves(fb, opp).empty()) continue;
                double dl_abs = min(get_time() + max(0.3, (v_end - get_time()) * 0.25), v_end);
                Deadline dl(dl_abs);
                if (vct_disproved(fb, opp, VCT_DEPTH, dl, vct_tt)) return mv;
            }
        }
    }

    try {
        auto path = search_vcf(fb, opp, VCF_DEPTH, sub_deadline(0.3), vcf_tt);
        if (path.has_value()) {
            Pos m = (*path)[0];
            set<Pos> cand6;
            BoardGuard guard(fb, m.first, m.second, opp);
            auto defs = five_point_cells_fb(fb, m.first, m.second, opp);
            auto def_cands = _defense_candidates(fb, color, defs);
            for (auto& p : def_cands) cand6.insert(p);

            auto top5 = top_moves_fb(fb, color, 5);
            for (auto& p : top5) cand6.insert(p);

            vector<Pos> defs6(cand6.begin(), cand6.end());
            sort(defs6.begin(), defs6.end(), [&](const Pos& a, const Pos& b) {
                if (color == BLACK) {
                    int da = _net_black_ban_delta(fb, a.first, a.second);
                    int db = _net_black_ban_delta(fb, b.first, b.second);
                    if (da != db) return da < db;
                }
                return quick_eval_fb(fb, a.first, a.second, color) > quick_eval_fb(fb, b.first, b.second, color);
                });
            if ((int)defs6.size() > 12) defs6.resize(12);

            double v_end = get_time() + deadline.left() * 0.5;
            for (auto& mv : defs6) {
                if (get_time() >= v_end || deadline.left() < 1.5) break;
                BoardGuard guard2(fb, mv.first, mv.second, color);
                if (!five_moves(fb, opp).empty()) continue;
                double dl_abs = min(get_time() + max(0.3, (v_end - get_time()) * 0.2), v_end);
                Deadline dl(dl_abs);
                if (vcf_disproved(fb, opp, VCF_DEPTH, dl, vcf_tt)) return mv;
            }
        }
    }
    catch (const TimeoutException&) {}

    try {
        auto path = search_vct_id(fb, color, VCT_DEPTH, sub_deadline(0.5), vct_tt);
        if (path.has_value()) return (*path)[0];
    }
    catch (const TimeoutException&) {}

    set<Pos> cand8;
    vector<Pos> defs8, ok8;
    try {
        auto path = search_vct_id(fb, opp, VCT_DEPTH, sub_deadline(0.4), vct_tt);
        if (path.has_value()) {
            vector<Pos> opp_starts;
            opp_starts.push_back((*path)[0]);
            auto top6 = top_moves_fb(fb, opp, 6);
            for (auto& p : top6) {
                if (find(opp_starts.begin(), opp_starts.end(), p) == opp_starts.end()) {
                    opp_starts.push_back(p);
                }
            }
            for (auto& s : opp_starts) {
                deadline.check();
                BoardGuard guard(fb, s.first, s.second, opp);
                auto res = analyze_move_k(fb, s.first, s.second, opp);
                set<Pos> base;
                if (get<2>(res) >= 1) {
                    auto fives = five_point_cells_fb(fb, s.first, s.second, opp);
                    base.insert(fives.begin(), fives.end());
                }
                else {
                    auto vic = _line_vicinity_fb(fb, s.first, s.second);
                    base.insert(vic.begin(), vic.end());
                }
                auto def_cands = _defense_candidates(fb, color, base);
                for (auto& p : def_cands) cand8.insert(p);
            }
            auto top5 = top_moves_fb(fb, color, 5);
            for (auto& p : top5) cand8.insert(p);

            defs8 = vector<Pos>(cand8.begin(), cand8.end());
            sort(defs8.begin(), defs8.end(), [&](const Pos& a, const Pos& b) {
                if (color == BLACK) {
                    int da = _net_black_ban_delta(fb, a.first, a.second);
                    int db = _net_black_ban_delta(fb, b.first, b.second);
                    if (da != db) return da < db;
                }
                return quick_eval_fb(fb, a.first, a.second, color) > quick_eval_fb(fb, b.first, b.second, color);
                });
            if ((int)defs8.size() > 10) defs8.resize(10);

            double v_end = get_time() + deadline.left() * 0.4;
            for (auto& mv : defs8) {
                if (get_time() >= v_end || deadline.left() < 1.5) break;
                BoardGuard guard2(fb, mv.first, mv.second, color);
                if (!five_moves(fb, opp).empty()) continue;
                double dl_abs = min(get_time() + max(0.4, (v_end - get_time()) * 0.25), v_end);
                Deadline dl(dl_abs);
                if (vct_disproved(fb, opp, VCT_DEPTH, dl, vct_tt)) {
                    Deadline dl_self(get_time() + max(0.3, deadline.left() * 0.15));
                    bool self_has_vcf = search_vcf(fb, color, VCF_DEPTH, dl_self, vcf_tt).has_value();
                    if (!self_has_vcf) {
                        Deadline dl_self2(get_time() + max(0.3, deadline.left() * 0.15));
                        self_has_vcf = search_vct_id(fb, color, 12, dl_self2, vct_tt).has_value();
                    }
                    if (self_has_vcf) return mv;
                    continue;
                }
                ok8.push_back(mv);
            }
        }
    }
    catch (const TimeoutException&) {}

    try {
        vector<tuple<int, Pos, int>> ca;
        auto fours = four_moves_full(fb, color);
        sort(fours.begin(), fours.end(), [](const pair<Pos, int>& a, const pair<Pos, int>& b) { return a.second > b.second; });
        int limit = min(8, (int)fours.size());
        for (int i = 0; i < limit; ++i) ca.push_back(make_tuple(0, fours[i].first, fours[i].second));

        auto threats = _threat_moves_scored(fb, color);
        for (auto& item : threats) {
            if (item.second >= 20) ca.push_back(make_tuple(1, item.first, 0));
        }
        if (ca.empty()) {
            auto threes = three_moves(fb, color);
            int lim3 = min(3, (int)threes.size());
            for (int i = 0; i < lim3; ++i) ca.push_back(make_tuple(2, threes[i], 0));
        }

        set<Pos> cand8_ref = cand8;
        sort(ca.begin(), ca.end(), [&](const tuple<int, Pos, int>& a, const tuple<int, Pos, int>& b) {
            if (get<0>(a) != get<0>(b)) return get<0>(a) < get<0>(b);
            bool a_in = cand8_ref.count(get<1>(a)) > 0;
            bool b_in = cand8_ref.count(get<1>(b)) > 0;
            return a_in > b_in;
            });
        if ((int)ca.size() > 10) ca.resize(10);

        double v_end2 = get_time() + deadline.left() * 0.35;
        optional<Pos> best_ca;
        optional<int> best_h;
        for (auto& item : ca) {
            int rank = get<0>(item);
            Pos mv = get<1>(item);
            int fp = get<2>(item);
            if (get_time() >= v_end2 || deadline.left() < 1.5) break;

            optional<int> h_opt;
            BoardGuard guard(fb, mv.first, mv.second, color);
            if (!five_moves(fb, opp).empty()) continue;

            double dl_abs = min(get_time() + max(0.3, (v_end2 - get_time()) * 0.3), v_end2);
            Deadline dl(dl_abs);
            if (vct_disproved(fb, opp, VCT_DEPTH, dl, vct_tt)) return mv;

            if (fp == 1) {
                auto qs = five_point_cells_fb(fb, mv.first, mv.second, color);
                bool race_win = !qs.empty();
                for (auto& q : qs) {
                    if (opp == BLACK && is_ban_move_fb(fb, q.first, q.second)) continue;
                    BoardGuard guard2(fb, q.first, q.second, opp);
                    double dl2_abs = min(get_time() + max(0.3, (v_end2 - get_time()) * 0.2), v_end2);
                    Deadline dl2(dl2_abs);
                    if (!search_vcf(fb, color, VCF_DEPTH, dl2, vcf_tt).has_value() && !search_vct_id(fb, color, 12, dl2, vct_tt).has_value()) {
                        race_win = false;
                    }
                }
                if (race_win) return mv;
            }
            h_opt = (int)four_moves_full(fb, opp).size();

            if (h_opt.has_value()) {
                if (!best_h.has_value() || h_opt.value() < best_h.value()) {
                    best_h = h_opt;
                    best_ca = mv;
                }
            }
        }
        if (best_ca.has_value()) return best_ca;
        if (!ok8.empty()) return _best_by_eval_fb(fb, ok8, color);
    }
    catch (const TimeoutException&) {}

    optional<Pos> best_mv;
    for (int d : ID_DEPTHS) {
        if (deadline.left() < 1.5) break;
        try {
            auto res = negamax(fb, color, d, -1e9, 1e9, deadline, 0, search_tt);
            if (res.second.has_value()) best_mv = res.second;
        }
        catch (const TimeoutException&) {
            break;
        }
    }
    if (best_mv.has_value()) return best_mv;
    return fallback_move_fb(fb, color);
}

// ================= 9. AI 封装 =================
double _budget(int stones, bool opp_threat) {
    if (stones <= 8) return TIMEOUT_LIMIT * 0.35;
    if (opp_threat) return TIMEOUT_LIMIT;
    return TIMEOUT_LIMIT * 0.8;
}

struct AI {
    vector<vector<int>> board;
    int color, opp;
    FastBoard fb;
    unordered_map<uint64_t, TTEntry> vcf_tt, vct_tt, search_tt;

    AI(const vector<vector<int>>& board_2d, int c) : board(board_2d), color(c), opp(1 - c), fb(board_2d) {}

    void sync_board(const vector<vector<int>>& board_2d) {
        board = board_2d;
        fb = FastBoard(board_2d);
    }

    optional<Pos> get_move(const vector<vector<int>>& board_2d, int c) {
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
};

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

// ================= 10. 框架交互逻辑 =================
string get_last_request(const string& json) {
    size_t pos = json.find("\"requests\"");
    if (pos != string::npos) {
        pos = json.find("[", pos);
        size_t end = json.find("]", pos);
        if (pos != string::npos && end != string::npos) {
            string arr = json.substr(pos, end - pos + 1);
            size_t last_obj = arr.rfind("{");
            size_t last_obj_end = arr.rfind("}");
            if (last_obj != string::npos && last_obj_end != string::npos) {
                return arr.substr(last_obj, last_obj_end - last_obj + 1);
            }
        }
    }
    size_t pos_req = json.find("\"request\"");
    if (pos_req != string::npos) {
        size_t start = json.find("{", pos_req);
        size_t end = json.rfind("}");
        if (start != string::npos && end != string::npos) {
            return json.substr(start, end - start + 1);
        }
    }
    return json;
}

vector<vector<int>> extract_board(const string& json) {
    vector<vector<int>> board(SIZE, vector<int>(SIZE, EMPTY));
    size_t pos = json.find("\"board\"");
    if (pos == string::npos) return board;
    pos = json.find("[", pos);
    int r = 0, c = 0;
    bool in_array = false;
    for (size_t i = pos; i < json.size() && r < SIZE; ++i) {
        if (json[i] == '[') {
            if (!in_array) in_array = true;
            else { c = 0; }
        }
        else if (json[i] == ']') {
            if (in_array) { in_array = false; r++; }
        }
        else if (json[i] == '0' || json[i] == '1') {
            if (in_array && r < SIZE && c < SIZE) {
                board[r][c++] = json[i] - '0';
            }
        }
    }
    return board;
}

int extract_int(const string& json, const string& key) {
    string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == string::npos) return -1;
    pos = json.find(":", pos);
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ')) pos++;
    size_t end = pos;
    while (end < json.size() && (isdigit(json[end]) || json[end] == '-')) end++;
    if (end > pos) return stoi(json.substr(pos, end - pos));
    return -1;
}

string extract_string(const string& json, const string& key) {
    string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == string::npos) return "";
    pos = json.find(":", pos);
    pos = json.find("\"", pos);
    if (pos == string::npos) return "";
    size_t end = json.find("\"", pos + 1);
    if (end == string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

vector<Pos> extract_candidates(const string& json) {
    vector<Pos> res;
    size_t pos = json.find("\"candidates\"");
    if (pos == string::npos) return res;
    pos = json.find("[", pos);
    size_t end = json.find("]", pos);
    if (pos == string::npos || end == string::npos) return res;
    string arr = json.substr(pos, end - pos + 1);
    for (size_t i = 0; i < arr.size(); ) {
        size_t p1 = arr.find("\"x\"", i);
        if (p1 == string::npos) break;
        size_t p2 = arr.find("\"y\"", p1);
        if (p2 == string::npos) break;
        size_t cx = arr.find(":", p1);
        size_t ex = arr.find_first_of(",}", cx);
        int x = stoi(arr.substr(cx + 1, ex - cx - 1));
        size_t cy = arr.find(":", p2);
        size_t ey = arr.find_first_of(",}", cy);
        int y = stoi(arr.substr(cy + 1, ey - cy - 1));
        res.push_back(make_pair(x, y));
        i = ey + 1;
    }
    return res;
}

Pos _transform(int k, Pos p) {
    int x = p.first, y = p.second;
    if (k == 0) return make_pair(x, y);
    if (k == 1) return make_pair(y, SIZE - 1 - x);
    if (k == 2) return make_pair(SIZE - 1 - x, SIZE - 1 - y);
    if (k == 3) return make_pair(SIZE - 1 - y, x);
    if (k == 4) return make_pair(x, SIZE - 1 - y);
    if (k == 5) return make_pair(SIZE - 1 - y, SIZE - 1 - x);
    if (k == 6) return make_pair(SIZE - 1 - x, y);
    return make_pair(y, x);
}

optional<OpeningVal> _lookup_opening(Pos w2, Pos b3) {
    for (int k = 0; k < 8; ++k) {
        Pos tw = _transform(k, w2);
        Pos tb = _transform(k, b3);
        auto it = OPENINGS_DB.find(make_pair(tw, tb));
        if (it != OPENINGS_DB.end()) return it->second;
    }
    return nullopt;
}

string _respond(const string& req_json, AI& ai) {
    string phase = extract_string(req_json, "phase");
    if (phase == PHASE_OPENING) {
        vector<OpeningKey> valid;
        for (auto& kv : OPENINGS_DB) {
            if (kv.second.second == "BALANCED" || kv.second.second == "BLACK_ADV") {
                valid.push_back(kv.first);
            }
        }
        if (valid.empty()) {
            for (auto& kv : OPENINGS_DB) valid.push_back(kv.first);
        }
        OpeningKey choice = valid[rand() % (int)valid.size()];
        stringstream ss;
        ss << "{\"action\": \"opening\", \"white2\": {\"x\": " << choice.first.first << ", \"y\": " << choice.first.second
            << "}, \"black3\": {\"x\": " << choice.second.first << ", \"y\": " << choice.second.second << "}, \"n\": 2}";
        return ss.str();
    }
    if (phase == PHASE_SWAP) {
        auto board = extract_board(req_json);
        if (board[SIZE / 2][SIZE / 2] != BLACK) {
            return "{\"action\": \"swap\", \"swap\": false}";
        }
        vector<Pos> whites, blacks;
        for (int i = 0; i < SIZE; ++i) {
            for (int j = 0; j < SIZE; ++j) {
                if (board[i][j] == WHITE) whites.push_back(make_pair(i, j));
                else if (board[i][j] == BLACK && !(i == SIZE / 2 && j == SIZE / 2)) blacks.push_back(make_pair(i, j));
            }
        }
        if (whites.size() != 1 || blacks.size() != 1) {
            return "{\"action\": \"swap\", \"swap\": false}";
        }
        auto entry = _lookup_opening(whites[0], blacks[0]);
        bool swap = entry.has_value() && (entry->second == "BLACK_WIN" || entry->second == "BLACK_ADV");
        stringstream ss;
        ss << "{\"action\": \"swap\", \"swap\": " << (swap ? "true" : "false") << "}";
        return ss.str();
    }

    ai.sync_board(extract_board(req_json));
    if (phase == PHASE_WHITE4) {
        auto mv = ai.get_move(ai.board, WHITE);
        stringstream ss;
        ss << "{\"action\": \"move\", \"x\": " << mv->first << ", \"y\": " << mv->second << "}";
        return ss.str();
    }
    if (phase == PHASE_BLACK5_CANDIDATES) {
        if (ai.color == BLACK) {
            auto pts = top_moves_fb(ai.fb, BLACK, 2);
            for (int i = 0; i < SIZE && (int)pts.size() < 2; ++i) {
                for (int j = 0; j < SIZE && (int)pts.size() < 2; ++j) {
                    Pos p = make_pair(i, j);
                    if (ai.fb.board[i][j] == EMPTY && find(pts.begin(), pts.end(), p) == pts.end() && _legal_fb(ai.fb, BLACK, p)) {
                        pts.push_back(p);
                    }
                }
            }
            size_t cnt = min((size_t)2, pts.size());
            stringstream ss;
            ss << "{\"action\": \"black5_candidates\", \"points\": [";
            for (size_t i = 0; i < cnt; ++i) {
                if (i) ss << ", ";
                ss << "{\"x\": " << pts[i].first << ", \"y\": " << pts[i].second << "}";
            }
            ss << "]}";
            return ss.str();
        }
        return "{\"action\": \"pass\"}";
    }
    if (phase == PHASE_BLACK5_SELECT) {
        auto pts = extract_candidates(req_json);
        int idx = 0;
        if ((int)pts.size() >= 2) {
            double s0 = evaluate_for_white_fb(ai.fb, pts[0]);
            double s1 = evaluate_for_white_fb(ai.fb, pts[1]);
            idx = (s0 >= s1) ? 0 : 1;
        }
        stringstream ss;
        ss << "{\"action\": \"black5_select\", \"index\": " << idx << "}";
        return ss.str();
    }
    if (phase.empty() || phase == PHASE_NORMAL) {
        auto mv = ai.get_move(ai.board, ai.color);
        stringstream ss;
        ss << "{\"action\": \"move\", \"x\": " << mv->first << ", \"y\": " << mv->second << "}";
        return ss.str();
    }
    return "{\"action\": \"pass\"}";
}

// ================= 11. 本地测试与主函数 =================
string normalize_pos(Pos pos) {
    return to_string(pos.first + 1) + (char)(pos.second + 'A');
}

void print_board(const vector<vector<int>>& board) {
    cout << "\n  ";
    for (int i = 0; i < SIZE; ++i) cout << "  " << (char)('A' + i);
    cout << "\n";
    for (int i = 0; i < SIZE; ++i) {
        cout << (i + 1 < 10 ? "  " : " ") << i + 1;
        for (int j = 0; j < SIZE; ++j) {
            if (board[i][j] == BLACK) cout << " X ";
            else if (board[i][j] == WHITE) cout << " O ";
            else cout << " + ";
        }
        cout << "\n";
    }
    cout << "\n";
}

string _strip(const string& s) {
    size_t st = s.find_first_not_of(" \t\r\n");
    if (st == string::npos) return "";
    size_t en = s.find_last_not_of(" \t\r\n");
    return s.substr(st, en - st + 1);
}

bool _all_digits(const string& s) {
    if (s.empty()) return false;
    for (char c : s) if (!isdigit((unsigned char)c)) return false;
    return true;
}

Pos get_user_move(const vector<vector<int>>& board, const string& prompt) {
    while (true) {
        cout << prompt;
        string s;
        getline(cin, s);
        s = _strip(s);

        if (!s.empty() && isalpha((unsigned char)s.back())) {
            string num_part = s.substr(0, s.length() - 1);
            char letter = toupper((unsigned char)s.back());
            if (_all_digits(num_part)) {
                int row = stoi(num_part) - 1;
                int col = letter - 'A';
                if (row >= 0 && row < SIZE && col >= 0 && col < SIZE && board[row][col] == EMPTY) {
                    return make_pair(row, col);
                }
            }
        }
        {
            istringstream iss(s);
            string a, b;
            if ((iss >> a >> b) && _all_digits(a) && _all_digits(b)) {
                int row = stoi(a), col = stoi(b);
                if (row >= 0 && row < SIZE && col >= 0 && col < SIZE && board[row][col] == EMPTY) {
                    return make_pair(row, col);
                }
            }
        }
        cout << "无效输入\n";
    }
}

bool _is_foul_play(const vector<vector<int>>& board, Pos m) {
    vector<vector<int>> b2 = board;
    b2[m.first][m.second] = EMPTY;
    FastBoard fb(b2);
    return is_ban_move_fb(fb, m.first, m.second);
}

int _read_int(const string& prompt, int default_v) {
    cout << prompt;
    string line;
    getline(cin, line);
    line = _strip(line);
    try {
        if (line.empty()) return default_v;
        return stoi(line);
    }
    catch (...) { return default_v; }
}

void run_local_test() {
    cout << "=== 本地对局测试 ===\n";
    cout << "选择身份 (b=黑/先手, w=白/后手): ";
    string choice;
    getline(cin, choice);
    transform(choice.begin(), choice.end(), choice.begin(), ::tolower);
    choice = _strip(choice);
    bool user_is_black = (choice == "b");

    vector<vector<int>> board(SIZE, vector<int>(SIZE, EMPTY));
    AI ai(board, user_is_black ? WHITE : BLACK);
    board[7][7] = BLACK;
    cout << "黑1 天元 " << normalize_pos(make_pair(7, 7)) << "\n";

    Pos w2, b3;
    if (user_is_black) {
        for (size_t i = 0; i < OPENINGS_ORDER.size(); ++i) {
            auto it_name = OPENINGS_DB.find(OPENINGS_ORDER[i]);
            string name = (it_name != OPENINGS_DB.end()) ? it_name->second.first : "?";
            cout << i << ": " << name << "\n";
        }
        int idx = _read_int("开局序号: ", 0);
        if (idx < 0 || idx >= (int)OPENINGS_ORDER.size()) idx = 0;
        w2 = OPENINGS_ORDER[idx].first;
        b3 = OPENINGS_ORDER[idx].second;
    }
    else {
        vector<OpeningKey> valid;
        for (auto& kv : OPENINGS_DB) {
            if (kv.second.second == "BALANCED" || kv.second.second == "BLACK_ADV") {
                valid.push_back(kv.first);
            }
        }
        if (valid.empty()) {
            for (auto& kv : OPENINGS_DB) valid.push_back(kv.first);
        }
        OpeningKey pick = valid[rand() % (int)valid.size()];
        w2 = pick.first;
        b3 = pick.second;
        auto it_name = OPENINGS_DB.find(make_pair(w2, b3));
        cout << "AI 选择开局: " << ((it_name != OPENINGS_DB.end()) ? it_name->second.first : "?") << "\n";
    }
    board[w2.first][w2.second] = WHITE;
    board[b3.first][b3.second] = BLACK;
    print_board(board);

    bool swap;
    if (user_is_black) {
        auto it_ov = OPENINGS_DB.find(make_pair(w2, b3));
        string st = (it_ov != OPENINGS_DB.end()) ? it_ov->second.second : "BALANCED";
        swap = (st == "BLACK_WIN" || st == "BLACK_ADV");
        cout << "AI 决定: " << (swap ? "交换" : "不交换") << "\n";
    }
    else {
        cout << "是否交换: ";
        string sw;
        getline(cin, sw);
        transform(sw.begin(), sw.end(), sw.begin(), ::tolower);
        sw = _strip(sw);
        swap = (sw == "y");
    }
    if (swap) user_is_black = !user_is_black;

    ai.color = user_is_black ? WHITE : BLACK;
    ai.opp = 1 - ai.color;
    ai.board = board;

    if (user_is_black) {
        auto m = ai.get_move(board, WHITE);
        Pos mv = m.value_or(make_pair(0, 0));
        board[mv.first][mv.second] = WHITE;
        cout << "AI(白) 下白4: " << normalize_pos(mv) << "\n";
    }
    else {
        Pos m = get_user_move(board, "白方(您)下白4 (如 8H): ");
        board[m.first][m.second] = WHITE;
    }
    print_board(board);

    Pos chosen;
    if (user_is_black) {
        Pos b5_1 = get_user_move(board, "黑5候选1 (如 8H): ");
        Pos b5_2 = get_user_move(board, "黑5候选2 (如 8H): ");
        chosen = (evaluate_for_white(board, b5_1) >= evaluate_for_white(board, b5_2)) ? b5_1 : b5_2;
        cout << "AI(白) 选择黑5 -> " << normalize_pos(chosen) << "\n";
    }
    else {
        vector<Pos> pts = top_moves(board, BLACK, 2);
        while ((int)pts.size() < 2) pts.push_back(make_pair(7, 6));
        cout << "AI(黑) 提供候选: 0=" << normalize_pos(pts[0])
            << ", 1=" << normalize_pos(pts[1]) << "\n";
        int idx = _read_int("白方(您)选择 (0/1): ", 0);
        if (idx < 0 || idx > 1) idx = 0;
        chosen = pts[idx];
    }
    board[chosen.first][chosen.second] = BLACK;
    print_board(board);

    int turn = WHITE;
    cout << "=== 进入正常对局 ===\n";
    while (true) {
        Pos m;
        if ((turn == BLACK) == user_is_black) {
            m = get_user_move(board, "轮到您 (如 8H): ");
        }
        else {
            auto ai_mv = ai.get_move(board, turn);
            m = ai_mv.value_or(make_pair(0, 0));
            cout << "AI(" << (turn == BLACK ? "黑" : "白") << ") 落子: " << normalize_pos(m) << "\n";
        }
        board[m.first][m.second] = turn;
        print_board(board);
        if (is_win_at(board, m, turn)) {
            cout << (turn == BLACK ? "黑" : "白") << "方获胜！\n";
            return;
        }
        if (turn == BLACK && _is_foul_play(board, m)) {
            cout << "黑方禁手! 白方获胜！\n";
            return;
        }
        turn = 1 - turn;
    }
}

string read_json() {
    string s, line;
    int depth = 0;
    bool in_string = false;
    while (getline(cin, line)) {
        for (char c : line) {
            if (c == '"' && (s.empty() || s.back() != '\\')) in_string = !in_string;
            if (!in_string) {
                if (c == '{' || c == '[') depth++;
                else if (c == '}' || c == ']') depth--;
            }
            s += c;
        }
        if (depth == 0 && !s.empty()) {
            size_t start = s.find_first_not_of(" \t\r\n");
            if (start != string::npos) return s.substr(start);
        }
    }
    return s;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    init_globals();

    if (LOCAL_TEST) {
        run_local_test();
        return 0;
    }

    unique_ptr<AI> ai;
    bool first = true;
    while (true) {
        string line = read_json();
        if (line.empty()) break;

        string resp_json = "{\"action\": \"pass\"}";
        try {
            string req = get_last_request(line);
            int c = extract_int(req, "color");
            if (c == -1) {
                if (ai) {
                    string ph = extract_string(req, "phase");
                    if (ph.empty() || ph == PHASE_NORMAL) {
                        auto b = extract_board(req);
                        int nb = 0, nw = 0;
                        for (int i = 0; i < SIZE; ++i)
                            for (int j = 0; j < SIZE; ++j) {
                                if (b[i][j] == BLACK) nb++;
                                else if (b[i][j] == WHITE) nw++;
                            }
                        c = (nb == nw) ? 0 : 1;
                    }
                    else {
                        c = ai->color;
                    }
                }
                else {
                    c = 0;
                }
            }
            int color = (c == 0) ? BLACK : WHITE;
            if (!ai) {
                ai = make_unique<AI>(extract_board(req), color);
            }
            else if (color != ai->color) {
                ai->color = color;
                ai->opp = 1 - color;
            }
            resp_json = _respond(req, *ai);
        }
        catch (const exception& e) {
            log_debug(string("Main loop exception: ") + e.what());
            if (ai) {
                auto mv = fallback_move(ai->board, ai->color);
                if (mv.has_value()) {
                    stringstream ss;
                    ss << "{\"action\": \"move\", \"x\": " << mv->first << ", \"y\": " << mv->second << "}";
                    resp_json = ss.str();
                }
            }
        }
        cout << "{\"response\": " << resp_json << "}\n" << flush;
        if (first) {
            first = false;
            cout << KEEP_RUNNING << "\n" << flush;
        }
    }
    return 0;
}