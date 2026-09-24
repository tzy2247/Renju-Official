#ifndef RENJU_CONFIG_H
#define RENJU_CONFIG_H

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <cmath>
#include <random>
#include <chrono>
#include <algorithm>
#include <optional>
#include <regex>
#include <sstream>
#include <cctype>
#include <tuple>
#include <memory>
#include <utility>

using namespace std;

// ================= 0. 全局配置 =================
const bool DEBUG_MODE = false;
const bool LOCAL_TEST = true;
const double TIMEOUT_LIMIT = 40.0;

const int SIZE = 15;
const int EMPTY = -1, BLACK = 0, WHITE = 1;
const string KEEP_RUNNING = ">>>BOTZONE_REQUEST_KEEP_RUNNING<<<";

const double WIN_SCORE = 1e7;
const double MATE_MARK = WIN_SCORE - 1000;
const int VCF_DEPTH = 36;
const int VCT_DEPTH = 24;
const int VCT_ID_START = 2;
const int VCT_ID_STEP = 2;
const int BRANCH = 10;
const vector<int> ID_DEPTHS = { 2, 4, 6, 8, 10 };

const int TT_CAP = 400000;
const int _AN_CAP = 150000;
const int _QE_CAP = 150000;
const int _KLINE_CAP = 250000;

const string PHASE_OPENING = "opening_proposal";
const string PHASE_SWAP = "swap_choice";
const string PHASE_WHITE4 = "white4";
const string PHASE_BLACK5_CANDIDATES = "black5_candidates";
const string PHASE_BLACK5_SELECT = "black5_select";
const string PHASE_NORMAL = "normal_play";

using Pos = pair<int, int>;
using OpeningKey = pair<Pos, Pos>;
using OpeningVal = pair<string, string>;
const char B0 = '0', B1 = '1', B2 = '2';

extern map<OpeningKey, OpeningVal> OPENINGS_DB;

extern const vector<OpeningKey> OPENINGS_ORDER;

#endif
