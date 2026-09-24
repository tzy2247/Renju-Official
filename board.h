#ifndef RENJU_BOARD_H
#define RENJU_BOARD_H

#include "config.h"
#include "utils.h"

// ================= 2. 预计算线表 & 棋盘 =================
struct FastBoard {
    int board[SIZE][SIZE];
    vector<string> line_strings;
    uint64_t hash;
    int near[SIZE][SIZE];
    int stones;
    int lcnt[100];
    uint64_t lkey[100];

    FastBoard();
    FastBoard(const vector<vector<int>>& board_2d);

    void place(int x, int y, int color);
    void undo(int x, int y, int color);
    string rel(int lid, int color) const;
    vector<Pos> candidates() const;
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

#endif
