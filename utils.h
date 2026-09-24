#ifndef RENJU_UTILS_H
#define RENJU_UTILS_H

#include <cstdint>

#include "config.h"

// ================= 1. 基础工具 =================
struct TimeoutException : public exception {};

double get_time();

struct Deadline {
    double t;
    Deadline(double t) : t(t) {}
    double left() const { return t - get_time(); }
    void check() const {
        if (get_time() > t) throw TimeoutException();
    }
};

void log_debug(const string& msg);

extern mt19937_64 rng;
extern uint64_t ZOBRIST[3][SIZE][SIZE];

extern vector<vector<Pos>> GLOBAL_LINES;
extern vector<vector<vector<Pos>>> CELL_LINES;
extern uint64_t LINE_Z[100][20][2];
extern int NUM_GLOBAL_LINES;

string swap_12(const string& s);

void init_globals();

#endif
