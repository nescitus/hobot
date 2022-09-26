// hobot.c -- Poor Go-playing engine

//bool useMercyBreak = true;

#include "hobot.h"

char         buf[BUFLEN];

double mcplayout(Position* pos, int amaf_map[], int owner_map[],
    int score_count[2 * N * N + 1], int disp)
{
    // Start a Monte Carlo playout from a given position, return score for to-play
    // player at the starting position; amaf_map is board-sized scratchpad recording
    // who played at a given position first

    if (random_int(10000) <= 0.10 * 10000.0)
        return random_playout(pos, amaf_map, owner_map, score_count, disp);
    else
        return hobot_playout(pos, amaf_map, owner_map, score_count, disp);
}

double random_playout(Position* pos, int amaf_map[], int owner_map[],
    int score_count[2 * N * N + 1], int disp)
{
    int depth = 0;
    double s = 0.0;
    int    passes = 0;
    Point  last_moves_neighbors[40], moves[BOARDSIZE], move;
    if (disp) {
        //disp_ladder = 1;
        fprintf(stderr, "** SIMULATION **\n");
    }

    if (board_nmoves(pos) > 0 && board_last_move(pos) == 0)
        passes = 1;

    while (passes < 2 && board_nmoves(pos) < MAX_GAME_LEN) {
        hobot_assert(pos, all_blocks_OK(pos));
        move = 0;
        if (disp) {
            fprintf(stderr, "mcplayout: idum = %u\n", idum);
            print_pos(pos, stderr, NULL);
        }

        // uniformly random move suggestions

        int x0 = random_int(N) + 1, y0 = random_int(N) + 1;
        move = choose_uniform_random_move(pos, y0 * (N + 1) + x0, disp);
    found:
        depth++;
        if (move == PASS_MOVE)  // No valid move : pass
        {     
            pass_move(pos);
            passes++;
        }
        else {
            if (amaf_map[move] == 0)      // mark the point with 1 for BLACK
            {                             // using WHITE because in hobot pos is updated after this line
                amaf_map[move] = (board_color_to_play(pos) == WHITE ? 1 : -1);
            }
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

    s = get_playout_score(pos, owner_map, score_count);
    return s;
}

double hobot_playout(Position* pos, int amaf_map[], int owner_map[],
    int score_count[2 * N * N + 1], int disp)
{
    int depth = 0;
    double s = 0.0;
    int    passes = 0;
    Point  last_moves_neighbors[40], moves[BOARDSIZE], move;
    if (disp) {
        //disp_ladder = 1;
        fprintf(stderr, "** SIMULATION **\n");
    }

    if (board_nmoves(pos) > 0 && board_last_move(pos) == 0)
        passes = 1;

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

        Color to_play = pos->to_play;
        Color to_not_play = color_other(to_play);

        // tonot first / 2: 447 467 457
        // tplay first /2: 474 488 443
        // tonot first raw 533

        // TODO: Escape heuristics suggestions

        if ((move = choose_side_tactics(pos, last_moves_neighbors,
            PROB_HEURISTIC_CAPTURE, disp, to_not_play)) != PASS_MOVE) {
            // sprintf(buf, "tonot");
            // log_fmt_s('PL', buf, NULL);
            goto found;
        }

        if ((move = choose_side_tactics(pos, last_moves_neighbors,
            PROB_HEURISTIC_CAPTURE, disp, to_play)) != PASS_MOVE) {
            //sprintf(buf, "toplay");
            //log_fmt_s('PL', buf, NULL);
            goto found;
        }

        // Capture heuristic suggestions

      /**  if ((move = choose_capture_move(pos, last_moves_neighbors,
            PROB_HEURISTIC_CAPTURE, disp)) != PASS_MOVE) {
            //sprintf(buf, "both");
            //log_fmt_s('PL', buf, NULL);
            goto found;
        }*/

        // Large pattern heuristic suggestions
        // (applied only near the beginning of a playout)

        if (depth < HEAVY_PLAYOUT_DEPTH && is_beyond_one_third == 0) {

            if (random_int(10000) <= 0.25 * 10000.0)
            {
                Point allmoves[BOARDSIZE];
                gen_potential_tree_moves(pos, allmoves, BOARD_IMIN - 1);
                move = choose_large_pattern_move(pos, allmoves, disp);

                if (move != PASS_MOVE)
                    goto found;
            }
        }

        // Connection heuristic moves
/**
        if (random_int(10000) <= 0.25 * 10000.0)
        {
            Point allmoves[BOARDSIZE];
            gen_potential_tree_moves(pos, allmoves, BOARD_IMIN - 1);

            if (gen_connections(pos, allmoves, 1.000, moves)) {
                mark_init(already_suggested);
                if ((move = choose_from(pos, moves, "connect", disp)) != PASS_MOVE) {
                    mark_release(already_suggested);
                    goto found;
                }

                mark_release(already_suggested);
            }
        }
/**/

        // 3x3 patterns heuristic suggestions

        if (gen_playout_moves_pat3(pos, last_moves_neighbors,
            PROB_HEURISTIC_PAT3, moves)) 
        {
            mark_init(already_suggested);
            if ((move = choose_from(pos, moves, "pat3", disp)) != PASS_MOVE) 
            {
                mark_release(already_suggested);
                goto found;
            }
            mark_release(already_suggested);
        }

        // random move 

        int x0 = random_int(N) + 1, y0 = random_int(N) + 1;
        move = choose_random_move(pos, y0 * (N + 1) + x0, disp);
    found:
        depth++;
        if (move == PASS_MOVE) {      // No valid move : pass
            pass_move(pos);
            passes++;
        }
        else {
            if (amaf_map[move] == 0)      // mark the point with 1 for BLACK
                // WHITE because in michi.py pos is updated after this line
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
    s = get_playout_score(pos, owner_map, score_count);
    return s;
}

int get_capture_score(Position* pos)
{
    return (pos->caps[1] - pos->caps[0] - board_komi(pos) - board_delta_komi(pos));
}

double get_playout_score(Position* pos, int owner_map[], int score_count[2 * N * N + 1])
// compute score (>0 if BLACK wins); this assumes a final position with all 
// dead stones captured and only single point eyes on the board ...
{
    double s1;
    int s = 0;

    FORALL_POINTS(pos, pt) {
        Color c = point_color(pos, pt);
        if (c == EMPTY) c = is_eyeish(pos, pt);
        if (c == BLACK) {
            s++;
            owner_map[pt]++;
        }
        else if (c == WHITE) {
            s--;
            owner_map[pt]--;
        }
    }
    s1 = s;
    score_count[s + N * N]++;
    return s1 - board_komi(pos) - board_delta_komi(pos);
}
