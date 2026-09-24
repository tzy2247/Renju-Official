#include "utils.h"

double get_time() {
    return chrono::duration<double>(chrono::steady_clock::now().time_since_epoch()).count();
}
void log_debug(const string& msg) {
    if (DEBUG_MODE) cerr << "[DEBUG] " << msg << endl;
}

mt19937_64 rng(1337);
uint64_t ZOBRIST[3][SIZE][SIZE];

vector<vector<Pos>> GLOBAL_LINES;
vector<vector<vector<Pos>>> CELL_LINES;
uint64_t LINE_Z[100][20][2];
int NUM_GLOBAL_LINES = 0;
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
