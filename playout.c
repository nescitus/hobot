// hobot.c -- Poor Go-playing engine

#include "hobot.h"

char buf[BUFLEN];

double mcplayout(Position* pos, int amaf_map[], int owner_map[],
    int score_count[2 * N * N + 1], int disp)
    // Start a Monte Carlo playout from a given position, return score for to-play
    // player at the starting position; amaf_map is board-sized scratchpad recording// who played at a given position first
{
    int depth = 0;
    double s = 0.0;
    int    passes = 0;
    Point  last_moves_neighbors[40], moves[BOARDSIZE], move;
    if (disp) 
    {
        fprintf(stderr, "** SIMULATION **\n");
    }
    if (board_nmoves(pos) > 0 && board_last_move(pos) == 0) passes = 1;

    while (passes < 2 && board_nmoves(pos) < MAX_GAME_LEN) {
        hobot_assert(pos, all_blocks_OK(pos));
        move = 0;
        if (disp) {
            fprintf(stderr, "mcplayout: idum = %u\n", idum);
            print_pos(pos, stderr, NULL);
        }
        // We simply try the moves our heuristics generate, in a particular
        // order, but not with 100% probability; this is on the border between
        // "rule-based playouts" and "probability distribution playouts".

        make_list_last_moves_neighbors(pos, last_moves_neighbors, 4);

        // Capture heuristic suggestions
        if ((move = choose_capture_move(pos, last_moves_neighbors,
            PROB_HEURISTIC_CAPTURE, disp)) != PASS_MOVE)
            goto found;

        /*
        if (depth < 16 && is_beyond_one_third == 0) {

            if (random_int(10000) <= 0.25 * 10000.0)
            {
                Point allmoves[BOARDSIZE];
                gen_potential_tree_moves(pos, allmoves, BOARD_IMIN - 1);

                if (gen_playout_moves_pat_large(pos, allmoves, 1.000, moves)) {
                    mark_init(already_suggested);
                    if ((move = choose_from(pos, moves, "pat", disp)) != PASS_MOVE) {
                        mark_release(already_suggested);
                        goto found;
                    }
                    mark_release(already_suggested);
                }
            }
        }
        */

        // ko

        Point pt = pos->ko;
        if (pt != PASS_MOVE && random_int(10000) <= 0.05 * 10000.0) {

            char* ret = play_move(pos, pt);

            // Possible failure reasons:
            // move is suicide, move retakes ko. 

            if (ret[0] != 0)
                continue;

            undo_move(pos, pt);
            move = pt;
            goto found;

        }

        // 3x3 patterns heuristic suggestions
        if (gen_playout_moves_pat3(pos, last_moves_neighbors,
            PROB_HEURISTIC_PAT3, moves)) {
            mark_init(already_suggested);
            if ((move = choose_from(pos, moves, "pat3", disp)) != PASS_MOVE) {
                mark_release(already_suggested);
                goto found;
            }
            mark_release(already_suggested);
        }

        int x0 = random_int(N) + 1, y0 = random_int(N) + 1;


        // random move suggestions
        // (called two additional times if selected move
        // is marked for rejection)

        pt = random_move_by_coors();
        if (is_rejected_in_playout(pos, pt))
            pt = random_move_by_coors();
        if (is_rejected_in_playout(pos, pt))
            pt = random_move_by_coors();

        move = choose_random_move(pos, y0 * (N + 1) + x0, disp);
    found:
        depth++;
        if (move == PASS_MOVE) {      // No valid move : pass
            pass_move(pos);
            passes++;
        }
        else {
            if (amaf_map[move] == 0)      // mark the point with 1 for BLACK
                                          // WHITE because pos is updated after this line
                amaf_map[move] = (board_color_to_play(pos) == WHITE ? 1 : -1);
            // TODO: make amaf premium depth-dependent
            passes = 0;
        }

        // mercy break

        double capt_score = get_capture_score(pos);

        if (capt_score > TERMINATE_PLAYOUT
        || capt_score < -TERMINATE_PLAYOUT) 
        {
            return capt_score;
        }

    }
    s = playout_score(pos, owner_map, score_count);
    return s;
}