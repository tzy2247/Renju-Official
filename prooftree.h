#ifndef RENJU_PROOFTREE_H
#define RENJU_PROOFTREE_H

#include "config.h"
#include "utils.h"
#include "board.h"
#include "analysis.h"
#include "tactics.h"
#include "eval.h"
#include "search.h"

// ================= 6.9 VCT 完整证明树 (防反击重写版) =================
struct VctNode {
    Pos atk{ -1, -1 };
    bool terminal{ false };
    Pos win_five{ -1, -1 };
    map<Pos, VctNode> replies;
    VctNode() = default;
    VctNode(const Pos& a, bool t, const Pos& w) : atk(a), terminal(t), win_five(w) {}
};

// 辅助函数：检查某一步棋是否形成了四或活三（即反击威胁）
bool is_counter_threat(FastBoard& fb, int x, int y, int color);

optional<VctNode> search_vct_tree(FastBoard& fb, int color, int depth,
    const Deadline& deadline, long long& node_budget);

bool verify_vct_tree(FastBoard& fb, const VctNode& node, int color, const Deadline& dl);

struct MateNode {
    Pos atk{ -1, -1 };
    bool terminal{ false };
    Pos win_five{ -1, -1 };
    map<Pos, MateNode> replies;
    MateNode() = default;
    MateNode(const Pos& a, bool t, const Pos& w) : atk(a), terminal(t), win_five(w) {}
};

optional<MateNode> search_vcf_tree(FastBoard& fb, int color, int depth,
    const Deadline& deadline, long long& node_budget);

bool verify_mate_tree(FastBoard& fb, const MateNode& node, int color, const Deadline& dl);

// ================= 6.10 快速绝杀模式 =================
extern optional<MateNode> g_mate_root;
extern const MateNode* g_mate_cur;
extern int g_mate_color;

void mate_mode_reset();
bool mate_mode_active();

void mate_mode_notify(FastBoard& fb, Pos opp_move);

optional<Pos> mate_mode_move(FastBoard& fb, int color);

#endif
