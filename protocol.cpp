#include "protocol.h"

string get_last_request(const string& json) {
    size_t pos = json.find("\"requests\"");
    if (pos != string::npos) {
        pos = json.find("[", pos);
        size_t end = json.find("]", pos);
        if (pos != string::npos && end != string::npos) {
            string arr = json.substr(pos, end - pos + 1);
            size_t last_obj = arr.rfind("{");
            size_t last_obj_end = arr.rfind("}");
            if (last_obj != string::npos && last_obj_end != string::npos) {
                return arr.substr(last_obj, last_obj_end - last_obj + 1);
            }
        }
    }
    size_t pos_req = json.find("\"request\"");
    if (pos_req != string::npos) {
        size_t start = json.find("{", pos_req);
        size_t end = json.rfind("}");
        if (start != string::npos && end != string::npos) {
            return json.substr(start, end - start + 1);
        }
    }
    return json;
}

vector<vector<int>> extract_board(const string& json) {
    vector<vector<int>> board(SIZE, vector<int>(SIZE, EMPTY));
    size_t pos = json.find("\"board\"");
    if (pos == string::npos) return board;
    pos = json.find("[", pos);
    int r = 0, c = 0;
    bool in_array = false;
    for (size_t i = pos; i < json.size() && r < SIZE; ++i) {
        if (json[i] == '[') {
            if (!in_array) in_array = true;
            else { c = 0; }
        }
        else if (json[i] == ']') {
            if (in_array) { in_array = false; r++; }
        }
        else if (json[i] == '0' || json[i] == '1') {
            if (in_array && r < SIZE && c < SIZE) {
                board[r][c++] = json[i] - '0';
            }
        }
    }
    return board;
}

int extract_int(const string& json, const string& key) {
    string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == string::npos) return -1;
    pos = json.find(":", pos);
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ')) pos++;
    size_t end = pos;
    while (end < json.size() && (isdigit(json[end]) || json[end] == '-')) end++;
    if (end > pos) return stoi(json.substr(pos, end - pos));
    return -1;
}

string extract_string(const string& json, const string& key) {
    string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == string::npos) return "";
    pos = json.find(":", pos);
    pos = json.find("\"", pos);
    if (pos == string::npos) return "";
    size_t end = json.find("\"", pos + 1);
    if (end == string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

vector<Pos> extract_candidates(const string& json) {
    vector<Pos> res;
    size_t pos = json.find("\"candidates\"");
    if (pos == string::npos) return res;
    pos = json.find("[", pos);
    size_t end = json.find("]", pos);
    if (pos == string::npos || end == string::npos) return res;
    string arr = json.substr(pos, end - pos + 1);
    for (size_t i = 0; i < arr.size(); ) {
        size_t p1 = arr.find("\"x\"", i);
        if (p1 == string::npos) break;
        size_t p2 = arr.find("\"y\"", p1);
        if (p2 == string::npos) break;
        size_t cx = arr.find(":", p1);
        size_t ex = arr.find_first_of(",}", cx);
        int x = stoi(arr.substr(cx + 1, ex - cx - 1));
        size_t cy = arr.find(":", p2);
        size_t ey = arr.find_first_of(",}", cy);
        int y = stoi(arr.substr(cy + 1, ey - cy - 1));
        res.push_back(make_pair(x, y));
        i = ey + 1;
    }
    return res;
}
Pos _transform(int k, Pos p) {
    int x = p.first, y = p.second;
    if (k == 0) return make_pair(x, y);
    if (k == 1) return make_pair(y, SIZE - 1 - x);
    if (k == 2) return make_pair(SIZE - 1 - x, SIZE - 1 - y);
    if (k == 3) return make_pair(SIZE - 1 - y, x);
    if (k == 4) return make_pair(x, SIZE - 1 - y);
    if (k == 5) return make_pair(SIZE - 1 - y, SIZE - 1 - x);
    if (k == 6) return make_pair(SIZE - 1 - x, y);
    return make_pair(y, x);
}
optional<OpeningVal> _lookup_opening(Pos w2, Pos b3) {
    for (int k = 0; k < 8; ++k) {
        Pos tw = _transform(k, w2);
        Pos tb = _transform(k, b3);
        auto it = OPENINGS_DB.find(make_pair(tw, tb));
        if (it != OPENINGS_DB.end()) return it->second;
    }
    return nullopt;
}
string _respond(const string& req_json, AI& ai) {
    string phase = extract_string(req_json, "phase");
    if (phase == PHASE_OPENING) {
        vector<OpeningKey> valid;
        for (auto& kv : OPENINGS_DB) {
            if (kv.second.second == "BALANCED" || kv.second.second == "BLACK_ADV") {
                valid.push_back(kv.first);
            }
        }
        if (valid.empty()) {
            for (auto& kv : OPENINGS_DB) valid.push_back(kv.first);
        }
        OpeningKey choice = valid[rand() % (int)valid.size()];
        stringstream ss;
        ss << "{\"action\": \"opening\", \"white2\": {\"x\": " << choice.first.first << ", \"y\": " << choice.first.second
            << "}, \"black3\": {\"x\": " << choice.second.first << ", \"y\": " << choice.second.second << "}, \"n\": 2}";
        return ss.str();
    }
    if (phase == PHASE_SWAP) {
        auto board = extract_board(req_json);
        if (board[SIZE / 2][SIZE / 2] != BLACK) {
            return "{\"action\": \"swap\", \"swap\": false}";
        }
        vector<Pos> whites, blacks;
        for (int i = 0; i < SIZE; ++i) {
            for (int j = 0; j < SIZE; ++j) {
                if (board[i][j] == WHITE) whites.push_back(make_pair(i, j));
                else if (board[i][j] == BLACK && !(i == SIZE / 2 && j == SIZE / 2)) blacks.push_back(make_pair(i, j));
            }
        }
        if (whites.size() != 1 || blacks.size() != 1) {
            return "{\"action\": \"swap\", \"swap\": false}";
        }
        auto entry = _lookup_opening(whites[0], blacks[0]);
        bool swap = entry.has_value() && (entry->second == "BLACK_WIN" || entry->second == "BLACK_ADV");
        stringstream ss;
        ss << "{\"action\": \"swap\", \"swap\": " << (swap ? "true" : "false") << "}";
        return ss.str();
    }

    ai.sync_board(extract_board(req_json));
    if (phase == PHASE_WHITE4) {
        auto mv = ai.get_move(ai.board, WHITE);
        stringstream ss;
        ss << "{\"action\": \"move\", \"x\": " << mv->first << ", \"y\": " << mv->second << "}";
        return ss.str();
    }
    if (phase == PHASE_BLACK5_CANDIDATES) {
        if (ai.color == BLACK) {
            auto pts = top_moves_fb(ai.fb, BLACK, 2);
            for (int i = 0; i < SIZE && (int)pts.size() < 2; ++i) {
                for (int j = 0; j < SIZE && (int)pts.size() < 2; ++j) {
                    Pos p = make_pair(i, j);
                    if (ai.fb.board[i][j] == EMPTY && find(pts.begin(), pts.end(), p) == pts.end() && _legal_fb(ai.fb, BLACK, p)) {
                        pts.push_back(p);
                    }
                }
            }
            size_t cnt = min((size_t)2, pts.size());
            stringstream ss;
            ss << "{\"action\": \"black5_candidates\", \"points\": [";
            for (size_t i = 0; i < cnt; ++i) {
                if (i) ss << ", ";
                ss << "{\"x\": " << pts[i].first << ", \"y\": " << pts[i].second << "}";
            }
            ss << "]}";
            return ss.str();
        }
        return "{\"action\": \"pass\"}";
    }
    if (phase == PHASE_BLACK5_SELECT) {
        auto pts = extract_candidates(req_json);
        int idx = 0;
        if ((int)pts.size() >= 2) {
            double s0 = evaluate_for_white_fb(ai.fb, pts[0]);
            double s1 = evaluate_for_white_fb(ai.fb, pts[1]);
            idx = (s0 >= s1) ? 0 : 1;
        }
        stringstream ss;
        ss << "{\"action\": \"black5_select\", \"index\": " << idx << "}";
        return ss.str();
    }
    if (phase.empty() || phase == PHASE_NORMAL) {
        auto mv = ai.get_move(ai.board, ai.color);
        stringstream ss;
        ss << "{\"action\": \"move\", \"x\": " << mv->first << ", \"y\": " << mv->second << "}";
        return ss.str();
    }
    return "{\"action\": \"pass\"}";
}
string read_json() {
    string s, line;
    int depth = 0;
    bool in_string = false;
    while (getline(cin, line)) {
        for (char c : line) {
            if (c == '"' && (s.empty() || s.back() != '\\')) in_string = !in_string;
            if (!in_string) {
                if (c == '{' || c == '[') depth++;
                else if (c == '}' || c == ']') depth--;
            }
            s += c;
        }
        if (depth == 0 && !s.empty()) {
            size_t start = s.find_first_not_of(" \t\r\n");
            if (start != string::npos) return s.substr(start);
        }
    }
    return s;
}
