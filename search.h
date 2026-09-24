#ifndef RENJU_SEARCH_H
#define RENJU_SEARCH_H

#include "config.h"
#include "utils.h"
#include "board.h"
#include "analysis.h"
#include "tactics.h"
#include "eval.h"

// ================= 6. VCF/VCT 证明树 =================
struct TTEntry {
    int depth;
    int flag;
    double val;
    optional<Pos> mv;
};

extern unordered_map<uint64_t, TTEntry> tt_vcf, tt_vct, tt_search;

vector<pair<Pos, int>> _threat_moves_scored(FastBoard& fb, int color);
vector<Pos> _threat_moves(FastBoard& fb, int color);
vector<Pos> _defense_candidates(FastBoard& fb, int color, const set<Pos>& block_pts);

optional<vector<Pos>> search_vcf(FastBoard& fb, int color, int depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt);
optional<vector<Pos>> search_vct(FastBoard& fb, int color, int depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt, set<pair<uint64_t, int>>* path_ptr = nullptr);

optional<vector<Pos>> search_vct_id(FastBoard& fb, int color, int max_depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt);
bool vcf_disproved(FastBoard& fb, int color, int max_depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt);

enum class VctResult { PROVEN, DISPROVEN, UNKNOWN };

VctResult vct_check(FastBoard& fb, int color, int max_depth, const Deadline& deadline, unordered_map<uint64_t, TTEntry>& tt);

#endif
