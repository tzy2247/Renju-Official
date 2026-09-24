#ifndef RENJU_PROTOCOL_H
#define RENJU_PROTOCOL_H

#include "config.h"
#include "utils.h"
#include "ai.h"

// ================= 10. 框架交互逻辑 =================
string get_last_request(const string& json);

vector<vector<int>> extract_board(const string& json);

int extract_int(const string& json, const string& key);

string extract_string(const string& json, const string& key);

vector<Pos> extract_candidates(const string& json);

Pos _transform(int k, Pos p);

optional<OpeningVal> _lookup_opening(Pos w2, Pos b3);

string _respond(const string& req_json, AI& ai);

string read_json();

#endif
