#include "decide.h"

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


    // 1. 立即成五  2. 挡对方成五  3. 自己双四
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

    // ===== 3.5 VCF 完整证明树：唯一有权开启快速绝杀模式的通道 =====
    {
        optional<MateNode> mate_tree;
        double vcf_time = max(deadline.left() * 0.45, 1.0);
        Deadline vcf_dl(min(get_time() + vcf_time, deadline.t - 0.8));
        for (int d = 2; d <= VCF_DEPTH; d += 2) {
            long long budget = 500000;
            try {
                auto r = search_vcf_tree(fb, color, d, vcf_dl, budget);
                if (r.has_value()) { mate_tree = move(r); break; }
            }
            catch (const TimeoutException&) { break; }
            if (vcf_dl.left() < 0.5) break;
        }
        if (mate_tree.has_value()) {
            bool ok = false;
            try { ok = verify_mate_tree(fb, *mate_tree, color, Deadline(get_time() + 5.0)); }
            catch (const TimeoutException&) { ok = false; }
            if (ok) {
                g_mate_root = move(mate_tree);
                g_mate_color = color;
                g_mate_cur = &(*g_mate_root);
                return g_mate_cur->atk;
            }
        }
    }

    // 对方双四防守
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
            }
        }
    }

    // ================= 收集根节点候选池 (Root Candidates) =================
    set<Pos> root_cands; // 【定义候选池】

    // ===== 1. VCT 完整证明树搜索 =====
    // 既然信任 VCT 树，如果找到解，直接返回，不经过后续思考
    try {
        optional<VctNode> vct_tree;
        double vct_time = max(deadline.left() * 0.4, 1.0);
        Deadline vct_dl(min(get_time() + vct_time, deadline.t - 0.8));

        for (int d = 2; d <= VCT_DEPTH; d += 2) {
            long long budget = 400000;
            try {
                auto r = search_vct_tree(fb, color, d, vct_dl, budget);
                if (r.has_value()) {
                    vct_tree = move(r);
                    break;
                }
            }
            catch (const TimeoutException&) { break; }
            if (vct_dl.left() < 0.5) break;
        }

        if (vct_tree.has_value()) {
            bool ok = false;
            try {
                ok = verify_vct_tree(fb, *vct_tree, color, Deadline(get_time() + 3.0));
            }
            catch (const TimeoutException&) {
                ok = false;
            }
            if (ok) {
                return vct_tree->atk; // 【直接返回】VCT 树已证明，无需再思考
            }
        }
    }
    catch (const TimeoutException&) {}

    // ===== 2. 防守对方 VCT (收集安全的防守点) =====
    set<Pos> cand8;
    vector<Pos> ok8;
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

            vector<Pos> defs8(cand8.begin(), cand8.end());
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

                auto res = vct_check(fb, opp, VCT_DEPTH, dl, vct_tt);
                if (res != VctResult::PROVEN) {
                    ok8.push_back(mv); // 对手没有 PROVEN 杀招，视为安全
                }
            }
        }
    }
    catch (const TimeoutException&) {}
    for (auto& p : ok8) root_cands.insert(p);

    // ===== 3. 启发式进攻候选评估 (大幅收紧，防止无证明树的盲目连杀) =====
    try {
        vector<tuple<int, Pos, int>> ca;

        if (!ca.empty()) {
            set<Pos> cand8_ref = cand8;
            sort(ca.begin(), ca.end(), [&](const tuple<int, Pos, int>& a, const tuple<int, Pos, int>& b) {
                if (get<0>(a) != get<0>(b)) return get<0>(a) < get<0>(b);
                bool a_in = cand8_ref.count(get<1>(a)) > 0;
                bool b_in = cand8_ref.count(get<1>(b)) > 0;
                return a_in > b_in;
                });
            if ((int)ca.size() > 5) ca.resize(5);

            double v_end2 = get_time() + deadline.left() * 0.35;
            for (auto& item : ca) {
                Pos mv = get<1>(item);
                if (get_time() >= v_end2 || deadline.left() < 1.5) break;

                BoardGuard guard(fb, mv.first, mv.second, color);
                if (!five_moves(fb, opp).empty()) continue;

                double dl_abs = min(get_time() + max(0.3, (v_end2 - get_time()) * 0.3), v_end2);
                Deadline dl(dl_abs);
                auto res = vct_check(fb, opp, VCT_DEPTH, dl, vct_tt);
                if (res != VctResult::PROVEN) {
                    root_cands.insert(mv); // 只有确认对手无杀，才勉强加入候选池
                }
            }
        }
    }
    catch (const TimeoutException&) {}

    // ===== 4. 补充基础候选，防止池子太小 =====
    auto top5 = top_moves_fb(fb, color, 5);
    for (auto& p : top5) root_cands.insert(p);

    // ===== 5. 最终决策：Negamax 深度校验 =====
    vector<Pos> final_cands(root_cands.begin(), root_cands.end());
    if (final_cands.empty()) {
        return fallback_move_fb(fb, color);
    }

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
