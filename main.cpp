#include "config.h"
#include "utils.h"
#include "protocol.h"
#include "local_test.h"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    init_globals();

    if (LOCAL_TEST) {
        run_local_test();
        return 0;
    }

    unique_ptr<AI> ai;
    bool first = true;
    while (true) {
        string line = read_json();
        if (line.empty()) break;

        string resp_json = "{\"action\": \"pass\"}";
        try {
            string req = get_last_request(line);
            int c = extract_int(req, "color");
            if (c == -1) {
                if (ai) {
                    string ph = extract_string(req, "phase");
                    if (ph.empty() || ph == PHASE_NORMAL) {
                        auto b = extract_board(req);
                        int nb = 0, nw = 0;
                        for (int i = 0; i < SIZE; ++i)
                            for (int j = 0; j < SIZE; ++j) {
                                if (b[i][j] == BLACK) nb++;
                                else if (b[i][j] == WHITE) nw++;
                            }
                        c = (nb == nw) ? 0 : 1;
                    }
                    else {
                        c = ai->color;
                    }
                }
                else {
                    c = 0;
                }
            }
            int color = (c == 0) ? BLACK : WHITE;
            if (!ai) {
                ai = make_unique<AI>(extract_board(req), color);
            }
            else if (color != ai->color) {
                ai->color = color;
                ai->opp = 1 - color;
            }
            resp_json = _respond(req, *ai);
        }
        catch (const exception& e) {
            log_debug(string("Main loop exception: ") + e.what());
            if (ai) {
                auto mv = fallback_move(ai->board, ai->color);
                if (mv.has_value()) {
                    stringstream ss;
                    ss << "{\"action\": \"move\", \"x\": " << mv->first << ", \"y\": " << mv->second << "}";
                    resp_json = ss.str();
                }
            }
        }
        cout << "{\"response\": " << resp_json << "}\n" << flush;
        if (first) {
            first = false;
            cout << KEEP_RUNNING << "\n" << flush;
        }
    }
    return 0;
}