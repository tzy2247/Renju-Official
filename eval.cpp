#include "eval.h"

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
