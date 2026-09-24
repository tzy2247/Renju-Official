#include "search.h"

unordered_map<uint64_t, TTEntry> tt_vcf, tt_vct, tt_search;

void _tt_store(unordered_map<uint64_t, TTEntry>& tt, uint64_t key, int depth, double val, optional<Pos> mv, int flag = 0) {
    if ((int)tt.size() > TT_CAP) tt.clear();
    tt[key] = { depth, flag, val, mv };
}

void _tt_store_fail(unordered_map<uint64_t, TTEntry>& tt, uint64_t key, int depth) {
    auto it = tt.find(key);
    if (it != tt.end() && !it->second.mv.has_value() && it->second.depth >= depth) return;
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
optional<vector<Pos>> search_vcf(FastBoard& fb, int color, int depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt) {
    deadline.check();
    uint64_t key = fb.hash ^ ((uint64_t)color << 50) ^ 0x12345678ULL;
    auto tt_result = _tt_get(tt, key, depth);
    bool hit = tt_result.first;
    optional<TTEntry> ent = tt_result.second;
    if (hit && ent.has_value() && ent->mv.has_value()) {
        return vector<Pos>{ent->mv.value()};
    }

    auto fives = five_moves(fb, color);
    if (!fives.empty()) {
        _tt_store(tt, key, depth, 0, fives[0], 0);
        return vector<Pos>{fives[0]};
    }
    if (depth <= 0) {
        _tt_store_fail(tt, key, depth);
        return nullopt;
    }
    int opp = 1 - color;
    auto fours = four_moves_full(fb, color);

    // 【跳四修复】双四 > 连冲四 > 跳四 > 估值
    // 注意：a.first 是 Pos，需要拆解为 a.first.first 和 a.first.second 传给 quick_eval_fb
    sort(fours.begin(), fours.end(), [&](const pair<Pos, int>& a, const pair<Pos, int>& b) {
        if (a.second != b.second) return a.second > b.second;
        int qa = _four_quality(fb, a.first, color);
        int qb = _four_quality(fb, b.first, color);
        if (qa != qb) return qa > qb;
        return quick_eval_fb(fb, a.first.first, a.first.second, color) > quick_eval_fb(fb, b.first.first, b.first.second, color);
        });

    for (auto& item : fours) {
        Pos atk = item.first;
        deadline.check();
        BoardGuard guard(fb, atk.first, atk.second, color);
        if (!five_moves(fb, opp).empty()) continue;
        auto defs = five_point_cells_fb(fb, atk.first, atk.second, color);
        if (defs.empty()) continue;
        bool all_win = true;
        for (auto& df : defs) {
            deadline.check();
            BoardGuard guard2(fb, df.first, df.second, opp);
            optional<vector<Pos>> sub;
            if (opp == BLACK) {
                auto res = analyze_move_k(fb, df.first, df.second, opp);
                if (get<0>(res)) sub = nullopt;
                else if (get<1>(res)) sub = vector<Pos>{ df };
                else sub = search_vcf(fb, color, depth - 1, deadline, tt);
            }
            else {
                sub = search_vcf(fb, color, depth - 1, deadline, tt);
            }
            if (!sub.has_value()) {
                all_win = false;
                break;
            }
        }
        if (all_win) {
            _tt_store(tt, key, depth, 0, atk, 0);
            return vector<Pos>{atk};
        }
    }
    _tt_store_fail(tt, key, depth);
    return nullopt;
}

optional<vector<Pos>> search_vct(FastBoard& fb, int color, int depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt, set<pair<uint64_t, int>>* path_ptr) {
    deadline.check();
    set<pair<uint64_t, int>> local_path;
    set<pair<uint64_t, int>>& path = path_ptr ? *path_ptr : local_path;
    uint64_t key_h = fb.hash ^ ((uint64_t)color << 50);
    if (path.count(make_pair(key_h, color))) return nullopt;

    uint64_t key = key_h ^ 0x87654321ULL;
    auto tt_result = _tt_get(tt, key, depth);
    bool hit = tt_result.first;
    optional<TTEntry> ent = tt_result.second;
    if (hit && ent.has_value() && ent->mv.has_value()) {
        return vector<Pos>{ent->mv.value()};
    }

    auto fives = five_moves(fb, color);
    if (!fives.empty()) {
        _tt_store(tt, key, depth, 0, fives[0], 0);
        return vector<Pos>{fives[0]};
    }
    if (depth <= 0) {
        _tt_store_fail(tt, key, depth);
        return nullopt;
    }
    int opp = 1 - color;

    auto fours_sorted = four_moves_full(fb, color);
    // 【跳四修复】双四 > 连冲四 > 跳四 > 估值
    sort(fours_sorted.begin(), fours_sorted.end(), [&](const pair<Pos, int>& a, const pair<Pos, int>& b) {
        if (a.second != b.second) return a.second > b.second;
        int qa = _four_quality(fb, a.first, color);
        int qb = _four_quality(fb, b.first, color);
        if (qa != qb) return qa > qb;
        return quick_eval_fb(fb, a.first.first, a.first.second, color) > quick_eval_fb(fb, b.first.first, b.first.second, color);
        });

    vector<Pos> atks;
    set<Pos> seen_atk;
    for (auto& item : fours_sorted) {
        seen_atk.insert(item.first);
        atks.push_back(item.first);
    }
    for (auto& p : _threat_moves(fb, color)) {
        if (seen_atk.find(p) == seen_atk.end()) {
            seen_atk.insert(p);
            atks.push_back(p);
        }
    }
    if (atks.empty()) {
        _tt_store_fail(tt, key, depth);
        return nullopt;
    }

    set<Pos> opp_fours_pre;
    for (auto& p : four_moves(fb, opp)) opp_fours_pre.insert(p);

    path.insert(make_pair(key_h, color));
    optional<vector<Pos>> result = nullopt;
    try {
        for (auto& atk : atks) {
            deadline.check();
            BoardGuard guard(fb, atk.first, atk.second, color);
            if (!five_moves(fb, opp).empty()) continue;
            auto res = analyze_move_k(fb, atk.first, atk.second, color);
            set<Pos> defs;
            if (get<2>(res) >= 1) {
                auto fives = five_point_cells_fb(fb, atk.first, atk.second, color);
                defs.insert(fives.begin(), fives.end());
            }
            else {
                auto vic = _line_vicinity_fb(fb, atk.first, atk.second);
                defs.insert(vic.begin(), vic.end());
            }
            for (auto& p : opp_fours_pre) {
                if (!(p == atk)) defs.insert(p);
            }
            if (defs.empty()) continue;
            bool all_win = true;
            for (auto& df : defs) {
                deadline.check();
                BoardGuard guard2(fb, df.first, df.second, opp);
                optional<vector<Pos>> sub;
                if (opp == BLACK) {
                    auto res_d = analyze_move_k(fb, df.first, df.second, opp);
                    if (get<0>(res_d)) sub = nullopt;
                    else if (get<1>(res_d)) sub = vector<Pos>{ df };
                    else sub = search_vct(fb, color, depth - 1, deadline, tt, &path);
                }
                else {
                    sub = search_vct(fb, color, depth - 1, deadline, tt, &path);
                }
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

VctResult vct_check(FastBoard& fb, int color, int max_depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt) {
    try {
        for (int d = VCT_ID_START; d <= max_depth; d += VCT_ID_STEP) {
            deadline.check();
            if (search_vct(fb, color, d, deadline, tt).has_value()) {
                return VctResult::PROVEN; // 找到了 VCT，证明对手有杀
            }
        }
        // 以现在的 search_vct (非四攻击未枚举所有应手)，无法证明 DISPROVEN
        return VctResult::UNKNOWN;
    }
    catch (const TimeoutException&) {
        return VctResult::UNKNOWN;
    }
}
