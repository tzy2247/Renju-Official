#include "negamax.h"

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
        double va = quick_eval_fb(fb, a.first, a.second, color) + 0.5 * hget(make_pair(color, a)) + 150.0 * _four_quality(fb, a, color);
        double vb = quick_eval_fb(fb, b.first, b.second, color) + 0.5 * hget(make_pair(color, b)) + 150.0 * _four_quality(fb, b, color);
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
