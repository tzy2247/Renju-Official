#include "board.h"

FastBoard::FastBoard() {
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
FastBoard::FastBoard(const vector<vector<int>>& board_2d) : FastBoard() {
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            if (board_2d[i][j] != EMPTY) {
                place(i, j, board_2d[i][j]);
            }
        }
    }
}
void FastBoard::place(int x, int y, int color) {
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
void FastBoard::undo(int x, int y, int color) {
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
string FastBoard::rel(int lid, int color) const {
    if (color == WHITE) return swap_12(line_strings[lid]);
    return line_strings[lid];
}
vector<Pos> FastBoard::candidates() const {
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
