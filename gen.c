// hobot.c -- Poor Go-playing engine

#include "hobot.h"

char buf[BUFLEN];

int gen_potential_tree_moves(Position* pos, Point moves[BOARDSIZE], Point i0)
// Generate a list of moves 
// - includes false positives - suicide moves;
// - does not include true-eye-filling moves, 
// - starts from a given board index (that can be used for randomization)
{
    Color c = board_color_to_play(pos);
    slist_clear(moves);

    for (Point i = i0; i < BOARD_IMAX; i++) {

        if (ignore_move(pos, i, c))
            continue;

        slist_push(moves, i);
    }

    for (Point i = BOARD_IMIN - 1; i < i0; i++) {

        if (ignore_move(pos, i, c))
            continue;

        slist_push(moves, i);
    }

    return slist_size(moves);
}

Point choose_side_tactics(Position* pos, Slist heuristic_set, float prob, int disp, int color)
{
    int   twolib_edgeonly = 1;
    Point move = PASS_MOVE, moves[20], sizes[20];

    if (random_int(10000) <= prob * 10000.0)
    {
        mark_init(already_suggested);
        FORALL_IN_SLIST(heuristic_set, pt)
            if (point_color(pos, pt) == color)
            {
                Block b = point_block(pos, pt);

                if (is_marked(already_suggested, b))
                    continue;

                mark(already_suggested, b);

                fix_atari(pos, pt, SINGLEPT_NOK, TWOLIBS_TEST,
                    twolib_edgeonly, moves, sizes);

                slist_shuffle(moves);

                move = choose_from(pos, moves, "tactics", disp);

                if (move != PASS_MOVE)
                    break;
            }

        mark_release(already_suggested);
    }
    return move;
}

Point choose_random_move(Position* pos, Point i0, int disp)
// Replace the sequence gen_playout_moves_random(); choose_from()
{
    char* ret;
    Color c = board_color_to_play(pos);
    Info     sizes[20];
    Point    ds[20], i = i0, move = PASS_MOVE;

    do {
        if (point_color(pos, i) != EMPTY)
            goto not_found;

        if (is_eye(pos, i) == c)
            goto not_found;  // ignore true eyes for player

        ret = play_move(pos, i);
        if (ret[0] == 0) {    // move OK
            move = i;
            // check if the suggested move did not turn out to be a self-atari
            int r = random_int(10000), tstrej;
            tstrej = r <= 10000.0 * PROB_RSAREJECT;
            if (tstrej) {
                slist_clear(ds); slist_clear(sizes);
                fix_atari(pos, i, SINGLEPT_OK, TWOLIBS_TEST, 1, ds, sizes);
                if (slist_size(ds) > 0) {
                    if (disp) fprintf(stderr, "rejecting self-atari move %s\n",
                        str_coord(i, buf));
                    undo_move(pos);
                    //*pos = saved_pos; // undo move;
                    move = PASS_MOVE;
                    goto not_found;
                }
            }
            break;
        }
    not_found:
        i++;
        if (i >= BOARD_IMAX) i = BOARD_IMIN - 1;
    } while (i != i0);
    return move;
}

Point choose_from(Position* pos, Slist moves, char* kind, int disp)
{
    char* ret;
    Info   sizes[20];
    Point  move = PASS_MOVE, ds[20];

    FORALL_IN_SLIST(moves, pt) {
        if (is_marked(already_suggested, pt))
            continue;
        mark(already_suggested, pt);
        if (disp && strcmp(kind, "random") != 0)
            fprintf(stderr, "move suggestion (%s) %s\n", kind, str_coord(pt, buf));
        ret = play_move(pos, pt);
        if (ret[0] == 0) {    // move OK
            move = pt;
            // check if the suggested move did not turn out to be a self-atari
            int r = random_int(10000), tstrej;
            if (strcmp(kind, "random") == 0) tstrej = r <= 10000.0 * PROB_RSAREJECT;
            else                            tstrej = r <= 10000.0 * PROB_SSAREJECT;
            if (tstrej) {
                slist_clear(ds); slist_clear(sizes);
                fix_atari(pos, pt, SINGLEPT_OK, TWOLIBS_TEST, 1, ds, sizes);
                if (slist_size(ds) > 0) {
                    if (disp) fprintf(stderr, "rejecting self-atari move %s\n",
                        str_coord(pt, buf));
                    undo_move(pos);
                    hobot_assert(pos, blocks_OK(pos, pt));
                    //*pos = saved_pos; // undo move;
                    move = PASS_MOVE;
                    continue;
                }
            }
            break;
        }
    }
    return move;
}

Point choose_capture_move(Position* pos, Slist heuristic_set, float prob, int disp)
// Replace the sequence gen_playout_capture_moves(); choose_from()
{
    int   twolib_edgeonly = 1;
    Point move = PASS_MOVE, moves[20], sizes[20];

    if (random_int(10000) <= prob * 10000.0) {
        mark_init(already_suggested);
        FORALL_IN_SLIST(heuristic_set, pt)
            if (point_is_color(pos, pt)) {
                Block b = point_block(pos, pt);
                if (is_marked(already_suggested, b))
                    continue;
                mark(already_suggested, b);
                fix_atari(pos, pt, SINGLEPT_NOK, TWOLIBS_TEST,
                    twolib_edgeonly, moves, sizes);
                slist_shuffle(moves);
                move = choose_from(pos, moves, "capture", disp);
                if (move != PASS_MOVE)
                    break;
            }
        mark_release(already_suggested);
    }
    return move;
}