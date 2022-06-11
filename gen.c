// hobot.c -- Poor Go-playing engine

#include "hobot.h"

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


int gen_playout_moves_capture(Position* pos, Slist heuristic_set, float prob,
    int expensive_ok, Slist moves, Slist sizes)
    // Compute list of candidate next moves in the order of preference (capture)
    // heuristic_set is the set of coordinates considered for applying heuristics;
    // this is the immediate neighborhood of last two moves in the playout, but
    // the whole board while prioring the tree.
{
    int   k, twolib_edgeonly = !expensive_ok;
    Point move2[20], size2[20];

    slist_clear(moves);
    slist_clear(sizes);
    if (random_int(10000) <= prob * 10000.0) {
        mark_init(already_suggested);
        FORALL_IN_SLIST(heuristic_set, pt)
            if (point_is_color(pos, pt)) {
                Block b = point_block(pos, pt); // pick block for analysis

                if (is_marked(already_suggested, b))
                    continue; // block already analysed!

                mark(already_suggested, b); // soon we will be one with this block

                // analyse capture/escape sequence
                fix_atari(pos, pt, SINGLEPT_NOK, TWOLIBS_TEST,
                    twolib_edgeonly, move2, size2);
                k = 1;

                // add capturing/escaping moves

                FORALL_IN_SLIST(move2, move)
                    if (slist_insert(moves, move))
                        slist_push(sizes, size2[k++]);
            }
        mark_release(already_suggested);
    }
    return slist_size(moves);
}

int gen_playout_moves_pat3(Position* pos, Slist heuristic_set, float prob,
    Slist moves)
    // Compute list of candidate next moves in the order of preference (3x3 pattern)
    // heuristic_set is the set of coordinates considered for applying heuristics;
    // this is the immediate neighborhood of last two moves in the playout, but
    // the whole board while prioring the tree.
{
    slist_clear(moves);
    if (random_int(10000) <= prob * 10000.0) {
        mark_init(already_suggested);
        FORALL_IN_SLIST(heuristic_set, pt) {
            if (point_color(pos, pt) == EMPTY) {
                if (pat3_match(pos, pt))
                    slist_push(moves, pt);
                //else if (get_min_libs(pos, pt) < 4) // CRASHES
                   // slist_push(moves, pt);
            }
        }

        mark_release(already_suggested);
    }
    return slist_size(moves);
}

int gen_playout_moves_pat_large(Position* pos, Slist heuristic_set, float prob,
    Slist moves)
{
    copy_to_large_board(pos);
    slist_clear(moves);
    if (random_int(10000) <= prob * 10000.0) {
        mark_init(already_suggested);
        FORALL_IN_SLIST(heuristic_set, pt) {
            if (point_color(pos, pt) == EMPTY) {
                int prob = large_pattern_prob_no_stats(pt, pos->ko);
                if (prob > 0.100 && prob < 99.0) {
                    slist_push(moves, pt);
                }
            }
        }

        mark_release(already_suggested);
    }
    return slist_size(moves);
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
