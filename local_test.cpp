#include "local_test.h"

string normalize_pos(Pos pos) {
    return to_string(pos.first + 1) + (char)(pos.second + 'A');
}

void print_board(const vector<vector<int>>& board) {
    cout << "\n  ";
    for (int i = 0; i < SIZE; ++i) cout << "  " << (char)('A' + i);
    cout << "\n";
    for (int i = 0; i < SIZE; ++i) {
        cout << (i + 1 < 10 ? "  " : " ") << i + 1;
        for (int j = 0; j < SIZE; ++j) {
            if (board[i][j] == BLACK) cout << " X ";
            else if (board[i][j] == WHITE) cout << " O ";
            else cout << " + ";
        }
        cout << "\n";
    }
    cout << "\n";
}

string _strip(const string& s) {
    size_t st = s.find_first_not_of(" \t\r\n");
    if (st == string::npos) return "";
    size_t en = s.find_last_not_of(" \t\r\n");
    return s.substr(st, en - st + 1);
}

bool _all_digits(const string& s) {
    if (s.empty()) return false;
    for (char c : s) if (!isdigit((unsigned char)c)) return false;
    return true;
}

Pos get_user_move(const vector<vector<int>>& board, const string& prompt) {
    while (true) {
        cout << prompt;
        string s;
        getline(cin, s);
        s = _strip(s);

        if (!s.empty() && isalpha((unsigned char)s.back())) {
            string num_part = s.substr(0, s.length() - 1);
            char letter = toupper((unsigned char)s.back());
            if (_all_digits(num_part)) {
                int row = stoi(num_part) - 1;
                int col = letter - 'A';
                if (row >= 0 && row < SIZE && col >= 0 && col < SIZE && board[row][col] == EMPTY) {
                    return make_pair(row, col);
                }
            }
        }
        {
            istringstream iss(s);
            string a, b;
            if ((iss >> a >> b) && _all_digits(a) && _all_digits(b)) {
                int row = stoi(a), col = stoi(b);
                if (row >= 0 && row < SIZE && col >= 0 && col < SIZE && board[row][col] == EMPTY) {
                    return make_pair(row, col);
                }
            }
        }
        cout << "无效输入\n";
    }
}

bool _is_foul_play(const vector<vector<int>>& board, Pos m) {
    vector<vector<int>> b2 = board;
    b2[m.first][m.second] = EMPTY;
    FastBoard fb(b2);
    return is_ban_move_fb(fb, m.first, m.second);
}

int _read_int(const string& prompt, int default_v) {
    cout << prompt;
    string line;
    getline(cin, line);
    line = _strip(line);
    try {
        if (line.empty()) return default_v;
        return stoi(line);
    }
    catch (...) { return default_v; }
}
void run_local_test() {
    cout << "=== 本地对局测试 ===\n";
    cout << "选择身份 (b=黑/先手, w=白/后手): ";
    string choice;
    getline(cin, choice);
    transform(choice.begin(), choice.end(), choice.begin(), ::tolower);
    choice = _strip(choice);
    bool user_is_black = (choice == "b");

    vector<vector<int>> board(SIZE, vector<int>(SIZE, EMPTY));
    AI ai(board, user_is_black ? WHITE : BLACK);
    board[7][7] = BLACK;
    cout << "黑1 天元 " << normalize_pos(make_pair(7, 7)) << "\n";

    Pos w2, b3;
    if (user_is_black) {
        for (size_t i = 0; i < OPENINGS_ORDER.size(); ++i) {
            auto it_name = OPENINGS_DB.find(OPENINGS_ORDER[i]);
            string name = (it_name != OPENINGS_DB.end()) ? it_name->second.first : "?";
            cout << i << ": " << name << "\n";
        }
        int idx = _read_int("开局序号: ", 0);
        if (idx < 0 || idx >= (int)OPENINGS_ORDER.size()) idx = 0;
        w2 = OPENINGS_ORDER[idx].first;
        b3 = OPENINGS_ORDER[idx].second;
    }
    else {
        vector<OpeningKey> valid;
        for (auto& kv : OPENINGS_DB) {
            if (kv.second.second == "BALANCED" || kv.second.second == "BLACK_ADV") {
                valid.push_back(kv.first);
            }
        }
        if (valid.empty()) {
            for (auto& kv : OPENINGS_DB) valid.push_back(kv.first);
        }
        OpeningKey pick = valid[rand() % (int)valid.size()];
        w2 = pick.first;
        b3 = pick.second;
        auto it_name = OPENINGS_DB.find(make_pair(w2, b3));
        cout << "AI 选择开局: " << ((it_name != OPENINGS_DB.end()) ? it_name->second.first : "?") << "\n";
    }
    board[w2.first][w2.second] = WHITE;
    board[b3.first][b3.second] = BLACK;
    print_board(board);

    bool swap;
    if (user_is_black) {
        auto it_ov = OPENINGS_DB.find(make_pair(w2, b3));
        string st = (it_ov != OPENINGS_DB.end()) ? it_ov->second.second : "BALANCED";
        swap = (st == "BLACK_WIN" || st == "BLACK_ADV");
        cout << "AI 决定: " << (swap ? "交换" : "不交换") << "\n";
    }
    else {
        cout << "是否交换: ";
        string sw;
        getline(cin, sw);
        transform(sw.begin(), sw.end(), sw.begin(), ::tolower);
        sw = _strip(sw);
        swap = (sw == "y");
    }
    if (swap) user_is_black = !user_is_black;

    ai.color = user_is_black ? WHITE : BLACK;
    ai.opp = 1 - ai.color;
    ai.board = board;

    if (user_is_black) {
        auto m = ai.get_move(board, WHITE);
        Pos mv = m.value_or(make_pair(0, 0));
        board[mv.first][mv.second] = WHITE;
        cout << "AI(白) 下白4: " << normalize_pos(mv) << "\n";
    }
    else {
        Pos m = get_user_move(board, "白方(您)下白4 (如 8H): ");
        board[m.first][m.second] = WHITE;
    }
    print_board(board);

    Pos chosen;
    if (user_is_black) {
        Pos b5_1 = get_user_move(board, "黑5候选1 (如 8H): ");
        Pos b5_2 = get_user_move(board, "黑5候选2 (如 8H): ");
        chosen = (evaluate_for_white(board, b5_1) >= evaluate_for_white(board, b5_2)) ? b5_1 : b5_2;
        cout << "AI(白) 选择黑5 -> " << normalize_pos(chosen) << "\n";
    }
    else {
        vector<Pos> pts = top_moves(board, BLACK, 2);
        while ((int)pts.size() < 2) pts.push_back(make_pair(7, 6));
        cout << "AI(黑) 提供候选: 0=" << normalize_pos(pts[0])
            << ", 1=" << normalize_pos(pts[1]) << "\n";
        int idx = _read_int("白方(您)选择 (0/1): ", 0);
        if (idx < 0 || idx > 1) idx = 0;
        chosen = pts[idx];
    }
    board[chosen.first][chosen.second] = BLACK;
    print_board(board);

    int turn = WHITE;
    cout << "=== 进入正常对局 ===\n";
    while (true) {
        Pos m;
        if ((turn == BLACK) == user_is_black) {
            m = get_user_move(board, "轮到您 (如 8H): ");
        }
        else {
            auto ai_mv = ai.get_move(board, turn);
            m = ai_mv.value_or(make_pair(0, 0));
            cout << "AI(" << (turn == BLACK ? "黑" : "白") << ") 落子: " << normalize_pos(m) << "\n";
        }
        board[m.first][m.second] = turn;
        print_board(board);
        if (is_win_at(board, m, turn)) {
            cout << (turn == BLACK ? "黑" : "白") << "方获胜！\n";
            return;
        }
        if (turn == BLACK && _is_foul_play(board, m)) {
            cout << "黑方禁手! 白方获胜！\n";
            return;
        }
        turn = 1 - turn;
    }
}
