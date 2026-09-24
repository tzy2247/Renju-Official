#ifndef RENJU_LOCAL_TEST_H
#define RENJU_LOCAL_TEST_H

#include "config.h"
#include "utils.h"
#include "ai.h"

// ================= 11. 本地测试与主函数 =================
string normalize_pos(Pos pos);

void print_board(const vector<vector<int>>& board);

string _strip(const string& s);

bool _all_digits(const string& s);

Pos get_user_move(const vector<vector<int>>& board, const string& prompt);

bool _is_foul_play(const vector<vector<int>>& board, Pos m);

int _read_int(const string& prompt, int default_v);

void run_local_test();

#endif
