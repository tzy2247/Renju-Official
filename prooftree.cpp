#include "prooftree.h"

bool is_counter_threat(FastBoard& fb, int x, int y, int color) {
    auto res = analyze_move_k(fb, x, y, color);
    // get<2> 是四的数量，get<4> 是活三的数量
    return get<2>(res) >= 1 || get<4>(res) >= 1;
}

optional<VctNode> search_vct_tree(FastBoard& fb, int color, int depth,
    const Deadline& deadline, long long& node_budget)
{
    deadline.check();
    if (--node_budget < 0) throw TimeoutException();

    auto fives = five_moves(fb, color);
    if (!fives.empty()) return VctNode(fives[0], true, fives[0]);
    if (depth <= 0) return nullopt;

    int opp = 1 - color;

    // 1. 优先尝试冲四 (Fours)
    auto fours = four_moves_full(fb, color);
    for (auto& item : fours) {
        const Pos& atk = item.first;
        if (color == BLACK && is_ban_move_fb(fb, atk.first, atk.second)) continue;

        BoardGuard guard(fb, atk.first, atk.second, color);
        if (!five_moves(fb, opp).empty()) continue; // 对手直接成五，无效

        auto fives_pts = five_point_cells_fb(fb, atk.first, atk.second, color);
        if (fives_pts.empty()) continue;

        VctNode node(atk, false, *fives_pts.begin());
        bool all_win = true;

        for (auto& df : fives_pts) {
            deadline.check();
            BoardGuard guard2(fb, df.first, df.second, opp);
            if (is_win_at_fb(fb, df.first, df.second, opp)) { all_win = false; break; }

            // 【关键修复】如果对手防守后形成了反击（四或活三），VCT 链条中断，此分支失败
            if (is_counter_threat(fb, df.first, df.second, opp)) {
                all_win = false; break;
            }

            auto sub = search_vct_tree(fb, color, depth - 2, deadline, node_budget);
            if (!sub.has_value()) { all_win = false; break; }
            node.replies[df] = move(*sub);
        }
        if (all_win) return node;
    }

    // 2. 尝试活三/做杀 (Threats)
    auto threats = _threat_moves_scored(fb, color);
    vector<pair<Pos, int>> pure_threats;
    for (auto& t : threats) {
        bool is_four = false;
        for (auto& f : fours) if (f.first == t.first) { is_four = true; break; }
        if (!is_four) pure_threats.push_back(t);
    }

    for (auto& item : pure_threats) {
        const Pos& atk = item.first;
        if (color == BLACK && is_ban_move_fb(fb, atk.first, atk.second)) continue;

        BoardGuard guard(fb, atk.first, atk.second, color);
        if (!five_moves(fb, opp).empty()) continue;

        set<Pos> replies;
        for (auto& p : fb.candidates()) {
            if (opp == BLACK && is_ban_move_fb(fb, p.first, p.second)) continue;
            replies.insert(p);
        }

        VctNode node(atk, false, Pos{ -1, -1 });
        bool all_win = true;

        for (auto& df : replies) {
            deadline.check();
            BoardGuard guard2(fb, df.first, df.second, opp);

            if (is_win_at_fb(fb, df.first, df.second, opp)) { all_win = false; break; }

            // 【关键修复】如果对手应手是反击（四或活三），VCT 链条中断
            if (is_counter_threat(fb, df.first, df.second, opp)) {
                all_win = false; break;
            }

            auto sub = search_vct_tree(fb, color, depth - 2, deadline, node_budget);
            if (!sub.has_value()) { all_win = false; break; }
            node.replies[df] = move(*sub);
        }
        if (all_win) return node;
    }

    return nullopt;
}
bool verify_vct_tree(FastBoard& fb, const VctNode& node, int color, const Deadline& dl) {
    dl.check();
    int opp = 1 - color;
    if (fb.board[node.atk.first][node.atk.second] != EMPTY) return false;
    if (color == BLACK && is_ban_move_fb(fb, node.atk.first, node.atk.second)) return false;

    BoardGuard g(fb, node.atk.first, node.atk.second, color);
    if (node.terminal) {
        return is_win_at_fb(fb, node.atk.first, node.atk.second, color);
    }
    if (is_win_at_fb(fb, node.atk.first, node.atk.second, color)) return false;

    auto res = analyze_move_k(fb, node.atk.first, node.atk.second, color);
    set<Pos> defs;
    bool is_four_attack = (get<2>(res) >= 1);

    if (is_four_attack) {
        auto fives_pts = five_point_cells_fb(fb, node.atk.first, node.atk.second, color);
        defs.insert(fives_pts.begin(), fives_pts.end());
    }
    else {
        for (auto& p : fb.candidates()) {
            if (opp == BLACK && is_ban_move_fb(fb, p.first, p.second)) continue;
            defs.insert(p);
        }
    }

    if (defs.empty()) return true;

    for (auto& df : defs) {
        if (fb.board[df.first][df.second] != EMPTY) continue;

        BoardGuard g2(fb, df.first, df.second, opp);
        if (is_win_at_fb(fb, df.first, df.second, opp)) return false;

        // 【关键验证】验证树必须再次确认对手应手不是反击
        if (is_counter_threat(fb, df.first, df.second, opp)) return false;

        auto it = node.replies.find(df);
        if (it != node.replies.end()) {
            if (!verify_vct_tree(fb, it->second, color, dl)) return false;
        }
        else {
            if (is_four_attack && node.win_five.first != -1) {
                if (fb.board[node.win_five.first][node.win_five.second] == EMPTY) {
                    BoardGuard g3(fb, node.win_five.first, node.win_five.second, color);
                    if (!is_win_at_fb(fb, node.win_five.first, node.win_five.second, color)) return false;
                }
                else { return false; }
            }
            else {
                return false;
            }
        }
    }
    return true;
}
optional<MateNode> search_vcf_tree(FastBoard& fb, int color, int depth,
    const Deadline& deadline, long long& node_budget)
{
    deadline.check();
    if (--node_budget < 0) throw TimeoutException();

    auto fives = five_moves(fb, color);
    if (!fives.empty()) return MateNode(fives[0], true, fives[0]);
    if (depth <= 0) return nullopt;

    int opp = 1 - color;
    auto fours = four_moves_full(fb, color);

    // 【跳四修复】双四 > 连冲四 > 跳四 > 估值
    sort(fours.begin(), fours.end(), [&](const pair<Pos, int>& a, const pair<Pos, int>& b) {
        if (a.second != b.second) return a.second > b.second;
        int qa = _four_quality(fb, a.first, color);
        int qb = _four_quality(fb, b.first, color);
        if (qa != qb) return qa > qb;
        return quick_eval_fb(fb, a.first.first, a.first.second, color)
                    > quick_eval_fb(fb, b.first.first, b.first.second, color);
        });

    for (auto& item : fours) {
        const Pos& atk = item.first;
        deadline.check();
        BoardGuard guard(fb, atk.first, atk.second, color);

        if (!five_moves(fb, opp).empty()) continue;

        auto five_pts = five_point_cells_fb(fb, atk.first, atk.second, color);
        if (five_pts.empty()) continue;

        MateNode node(atk, false, *five_pts.begin());
        bool all_win = true;

        for (auto& d : five_pts) {
            deadline.check();
            BoardGuard guard2(fb, d.first, d.second, opp);

            if (is_win_at_fb(fb, d.first, d.second, opp)) { all_win = false; break; }

            if (opp == BLACK) {
                auto res = analyze_move_k(fb, d.first, d.second, opp);
                if (get<1>(res)) continue;
            }

            auto sub = search_vcf_tree(fb, color, depth - 1, deadline, node_budget);
            if (!sub.has_value()) { all_win = false; break; }
            node.replies[d] = move(*sub);
        }
        if (all_win) return node;
    }
    return nullopt;
}
bool verify_mate_tree(FastBoard& fb, const MateNode& node, int color, const Deadline& dl) {
    dl.check();
    int opp = 1 - color;
    if (fb.board[node.atk.first][node.atk.second] != EMPTY) return false;

    BoardGuard g(fb, node.atk.first, node.atk.second, color);
    bool atk_wins = is_win_at_fb(fb, node.atk.first, node.atk.second, color);
    if (node.terminal) return atk_wins;
    if (atk_wins) return false;
    if (color == BLACK) {
        auto res = analyze_move_k(fb, node.atk.first, node.atk.second, color);
        if (get<1>(res)) return false;
    }

    auto five_pts = five_point_cells_fb(fb, node.atk.first, node.atk.second, color);
    if (five_pts.empty()) return false;
    if (five_pts.find(node.win_five) == five_pts.end()) return false;
    if (!five_moves(fb, opp).empty()) return false;

    set<Pos> defs = five_pts;
    for (auto& p : fb.candidates()) defs.insert(p);

    for (auto& d : defs) {
        if (fb.board[d.first][d.second] != EMPTY) continue;
        if (opp == BLACK && is_ban_move_fb(fb, d.first, d.second)) continue;

        BoardGuard g2(fb, d.first, d.second, opp);
        if (is_win_at_fb(fb, d.first, d.second, opp)) return false;

        auto it = node.replies.find(d);
        if (it != node.replies.end()) {
            if (!verify_mate_tree(fb, it->second, color, dl)) return false;
        }
        else {
            if (fb.board[node.win_five.first][node.win_five.second] != EMPTY) return false;
            BoardGuard g3(fb, node.win_five.first, node.win_five.second, color);
            if (!is_win_at_fb(fb, node.win_five.first, node.win_five.second, color))
                return false;
        }
    }
    return true;
}
// ================= 6.10 快速绝杀模式 =================
static MateNode g_mate_dev_node;
optional<MateNode> g_mate_root;
const MateNode* g_mate_cur = nullptr;
int g_mate_color = -1;

void mate_mode_reset() { g_mate_root.reset(); g_mate_cur = nullptr; g_mate_color = -1; }
bool mate_mode_active() { return g_mate_root.has_value(); }

void mate_mode_notify(FastBoard& fb, Pos opp_move) {
    if (!mate_mode_active() || g_mate_cur == nullptr) return;
    if (fb.board[g_mate_cur->atk.first][g_mate_cur->atk.second] != g_mate_color) {
        mate_mode_reset(); return;
    }
    if (g_mate_cur->terminal) return;
    auto it = g_mate_cur->replies.find(opp_move);
    if (it != g_mate_cur->replies.end()) {
        g_mate_cur = &it->second;
        return;
    }
    Pos w = g_mate_cur->win_five;
    if (fb.board[w.first][w.second] != EMPTY) { mate_mode_reset(); return; }
    BoardGuard g(fb, w.first, w.second, g_mate_color);
    if (!is_win_at_fb(fb, w.first, w.second, g_mate_color)) { mate_mode_reset(); return; }
    g_mate_dev_node = MateNode(w, true, w);
    g_mate_cur = &g_mate_dev_node;
}

optional<Pos> mate_mode_move(FastBoard& fb, int color) {
    if (!mate_mode_active() || g_mate_color != color || g_mate_cur == nullptr) return nullopt;
    const MateNode* n = g_mate_cur;
    if (fb.board[n->atk.first][n->atk.second] != EMPTY) return nullopt;
    if (n->terminal) {
        BoardGuard g(fb, n->atk.first, n->atk.second, color);
        if (!is_win_at_fb(fb, n->atk.first, n->atk.second, color)) return nullopt;
    }
    else {
        if (color == BLACK && is_ban_move_fb(fb, n->atk.first, n->atk.second)) return nullopt;
        BoardGuard g(fb, n->atk.first, n->atk.second, color);
        if (five_point_cells_fb(fb, n->atk.first, n->atk.second, color).empty()) return nullopt;
    }
    return n->atk;
}
