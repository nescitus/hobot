// hobot.c -- Poor Go-playing engine
/*
Hobot is based on a C recoding of the michi.py code by Petr Baudis
avalaible at https://github.com/pasky/michi .

(c) 2015 Petr Baudis <pasky@ucw.cz> Denis Blumstein <db3108@free.fr>
MIT licence (i.e. almost public domain)

(c) 2022 Pawel Koziol <pp.koziol@gmail.com>
*/
#include "hobot.h"

double URGENT_PATTERN_SCORE = 0.200; // triggers urgent move scenario
double TENUKI_PATTERN_SCORE = 0.199; // allows tenuki in urgent move scenario
double MERCY_LIMIT = 75;

//========================= Definition of Data Structures =====================
// Given a board of size NxN (N=9, 19, ...), we represent the position
// as an (N+1)*(N+2)+1 Byte array, with 0 (empty), 2 (white), 3 (black), 
// and 1 (off-board border to make rules implementation easier).  
// Coordinates are just indices in this array.
//
// -------------------------------- Global Data -------------------------------
Mark         *mark1, *mark2, *already_suggested;
static int   disp_ladder;
static char* colstr  = "@ABCDEFGHJKLMNOPQRST";
int is_beyond_halfway = 0;
int is_beyond_one_third = 0;

// Stack of Positions for use in recursive calls fix_atari/read_ladder_attack
int avail_pos;
Position stack_pos[500];

//================================ Go heuristics ==============================
// The couple of functions read_ladder_attack / fix_atari is maybe the most 
// complicated part of the whole program (sadly). 
// Feel free to just TREAT IT AS A BLACK-BOX, it's not really that interesting!
//
int fix_atari_r(Position *pos, Point pt, Slist moves);

__INLINE__ Position *push_position(Position *pos)
{
    Position *newpos = stack_pos + avail_pos++;
    if (avail_pos > 500)
        fatal_error("stack of Position overflow (> 500)");
    memcpy(newpos, pos, sizeof(Position));
    return newpos;
}

__INLINE__ void pop_position(void)
{
    avail_pos--;
}

Point read_ladder_attack(Position *pos, Point pt, Slist libs)
// Check if a capturable ladder is being pulled out at pt and return a move
// that continues it in that case. Expects its two liberties in libs.
// Actually, this is a general 2-lib capture exhaustive solver.
{
    char *ret;
    Point moves[100];
    Point l1, l2, move=PASS_MOVE;

    // always play first on the liberty that has 3 neighbors EMPTY
    if (point_nlibs(pos, libs[1]) == 3) {
        l2 = libs[1];
        goto process_l2;
    }
    if (point_nlibs(pos, libs[2]) == 3) {
        l2 = libs[2];
        goto process_l2;
    }
            
    if (line_height(libs[1], board_size(pos)) > 0) { 
        // for ladder try first the liberty that is not on the first line
        l1 = libs[1];
        l2 = libs[2];
    }
    else {
        l1 = libs[2];
        l2 = libs[1];
    }

    Position *pos_l = push_position(pos);
    if (disp_ladder) fprintf(stderr,"read_ladder_attack:\n");
    ret = play_move(pos_l, l1);
    if (ret[0]==0) {         // move is legal
        if (disp_ladder) print_pos(pos_l, stderr, 0);
        // fix_atari_r() will recursively call read_ladder_attack() back
        slist_clear(moves);
        int is_atari = fix_atari_r(pos_l, pt, moves);
        // if block is in atari and cannot escape, it is caugth in a ladder
        if (is_atari && slist_size(moves) == 0) {
            move = l1;
            pop_position();
            goto finished;
        }
    }
    pop_position();

process_l2:     // exploration can be done in pos which is a workspace
    if (disp_ladder) fprintf(stderr,"read_ladder_attack:\n");
    ret = play_move(pos, l2);
    if (ret[0]==0) {         // move is legal
        if (disp_ladder) print_pos(pos, stderr, 0);
        // fix_atari_r() will recursively call read_ladder_attack() back
        slist_clear(moves);
        int is_atari = fix_atari_r(pos, pt, moves);
        // if block is in atari and cannot escape, it is caugth in a ladder
        if (is_atari && slist_size(moves) == 0)
            move = l2;
    }
finished:
    return move;
}

int fix_atari_r(Position *pos, Point pt, Slist moves)
// An atari/capture analysis routine that checks the group at Point pt,
// determining whether (i) it is in atari (ii) if it can escape it,
// either by playing on its liberty or counter-capturing another group.
//
// Return 1 (true) if the group is in atari, 0 otherwise
//        moves : a list of moves that capture or save blocks
//        sizes : list of same lenght as moves (size of corresponding blocks)
// singlept_ok!=0 means that we will not try to save one-point groups
//
// Only called by read_ladder_attack()
{
    Block b = point_block(pos, pt);
    int   in_atari=1;
    Point l, libs[5], blocks[256], blibs[5];

    slist_clear(moves);
    if (block_nlibs(pos,b) >= 2) { 
        return 0;  
    }

    Color other=color_other(board_color_to_play(pos));
    block_compute_libs(pos, b, libs, 1);
    if (point_color(pos, pt) == other) { 
        // - this is opponent's group, that's enough to capture it
        //fprintf(stderr, "fix_atari_r: opponent block\n");
        slist_insert(moves, libs[1]); 
        return in_atari;
    }

    // This is our group and it is in atari
    // Before thinking about defense, what about counter-capturing a neighbor ?
    make_list_neighbor_blocks_in_atari(pos, b, blocks, pt);
    FORALL_IN_SLIST(blocks, b1) {
        block_compute_libs(pos, b1, blibs, 1);
        slist_insert(moves, blibs[1]);
    }

    l = libs[1];
    // We are escaping.  
    // Will playing our last liberty gain/ at least two liberties?
    char *ret = play_move(pos, l);

    if (ret[0]!=0)
        return 1;     // oops, suicidal move
    
    b = point_block(pos, l);
    if (block_nlibs(pos,b) >= 2) {
        // Good, there is still some liberty remaining - but if it's just the 
        // two, check that we are not caught in a ladder... (Except that we 
        // don't care if we already have some alternative escape routes!)
        if (slist_size(moves)>0 || block_nlibs(pos,b)>=3) { 
            slist_insert(moves, l);
        }
        else if (block_nlibs(pos,b)==2) {
            block_compute_libs(pos,b,libs,2);
            if (read_ladder_attack(pos,l,libs) == 0)
                slist_insert(moves, l);
        }
    }
    return in_atari;
}

int fix_atari(Position *pos, Point pt, int singlept_ok
        , int twolib_test, int twolib_edgeonly, Slist moves, Slist sizes)
// An atari/capture analysis routine that checks the group at Point pt,
// determining whether (i) it is in atari (ii) if it can escape it,
// either by playing on its liberty or counter-capturing another group.
//
// Return 1 (true) if the group is in atari, 0 otherwise
//        moves : a list of moves that capture or save blocks
//        sizes : list of same lenght as moves (size of corresponding blocks)
// singlept_ok!=0 means that we will not try to save one-point groups
{
    Block b = point_block(pos, pt);
    int   in_atari=1, maxlibs=3;
    Point l, libs[5], blocks[256], blibs[5];

    slist_clear(moves); slist_clear(sizes);
    
    if (singlept_ok && block_size(pos, b) == 1) 
        return 0;
 
    if (block_nlibs(pos,b) >= 2) { 
        if (twolib_test && block_nlibs(pos,b) == 2 && block_size(pos,b) > 1) {
            block_compute_libs(pos, b, libs, maxlibs);
            if (twolib_edgeonly
            && ((line_height(libs[1], board_size(pos)))>0 
                 || (line_height(libs[2], board_size(pos)))>0)) {
                // no expensive ladder check
                return 0;
            }
            else {
                // check that the block cannot be caught in a working ladder
                // If it can, that's as good as in atari, a capture threat.
                // (Almost - N/A for countercaptures.)
                Position workpos = *pos;
                Point ladder_attack = read_ladder_attack(&workpos, pt, libs);
                if (ladder_attack) {
                    if(slist_insert(moves, ladder_attack))
                        slist_push(sizes, block_size(pos, b));
                }
            }
        } 
        return 0;  
    }

    block_compute_libs(pos, b, libs, maxlibs);
    Color other=color_other(board_color_to_play(pos));
    if (point_color(pos, pt) == other) { 
        // - this is opponent's group, that's enough to capture it
        if (slist_insert(moves, libs[1])) 
            slist_push(sizes, block_size(pos, b));
        return in_atari;
    }

    // This is our group and it is in atari
    // Before thinking about defense, what about counter-capturing a neighbor ?
    make_list_neighbor_blocks_in_atari(pos, b, blocks, pt);
    FORALL_IN_SLIST(blocks, b1) {
        block_compute_libs(pos, b1, blibs, 1);
        slist_insert(moves, blibs[1]);
    }
    slist_sort(moves);
    FORALL_IN_SLIST(moves, n)
        slist_push(sizes,block_size(pos, point_block(pos, n)));

    l = libs[1];
    // We are escaping.  
    // Will playing our last liberty gain/ at least two liberties?
    char *ret = play_move(pos, l);
    
    if (ret[0]!=0)
        return 1;     // oops, suicidal move

    b = point_block(pos, l);
    if (block_nlibs(pos,b) >= 2) {
        // Good, there is still some liberty remaining - but if it's just the 
        // two, check that we are not caught in a ladder... (Except that we 
        // don't care if we already have some alternative escape routes!)
        if (slist_size(moves)>0 || block_nlibs(pos,b)>=3) { 
            if (slist_insert(moves, l))
                slist_push(sizes, block_size(pos, b));
        }
        else if (block_nlibs(pos,b)==2) {
            block_compute_libs(pos,b,libs,2);
            Position workpos = *pos;     // workspace for read_ladder_attack
            if (read_ladder_attack(&workpos, l, libs) == 0)
                if (slist_insert(moves, l))
                    slist_push(sizes, block_size(pos, b));
        }
    }
    undo_move(pos);
    return in_atari;
}

//========================= Montecarlo playout policy =========================
int gen_playout_moves_capture(Position *pos, Slist heuristic_set, float prob,
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
    if (random_int(10000) <= prob*10000.0) {
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
                k=1;

                // add capturing/escaping moves

                FORALL_IN_SLIST(move2, move)
                    if (slist_insert(moves, move))
                        slist_push(sizes, size2[k++]);
            }
        mark_release(already_suggested);
    }
    return slist_size(moves);
}

int gen_playout_moves_pat_large(Position* pos, Slist heuristic_set, float prob,
    Slist moves)
    // Compute list of large pattern moves with sufficiently high score
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

int gen_playout_moves_pat3(Position *pos, Slist heuristic_set, float prob,
                                                                Slist moves)
// Compute list of candidate next moves in the order of preference (3x3 pattern)
// heuristic_set is the set of coordinates considered for applying heuristics;
// this is the immediate neighborhood of last two moves in the playout, but
// the whole board while prioring the tree.
{
    slist_clear(moves);
    if (random_int(10000) <= prob*10000.0) {
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

int gen_potential_tree_moves(Position *pos, Point moves[BOARDSIZE], Point i0)
// Generate a list of moves 
// - includes false positives - suicide moves;
// - does not include true-eye-filling moves, 
// - starts from a given board index (that can be used for randomization)
{
    Color c = board_color_to_play(pos);
    slist_clear(moves);

    for(Point i = i0 ; i < BOARD_IMAX ; i++) {

        if (ignore_move(pos,i,c)) 
            continue;          

        slist_push(moves, i); 
    }
    
    for(Point i=BOARD_IMIN-1 ; i<i0 ; i++) {

        if (ignore_move(pos,i, c)) 
            continue;   
             
        slist_push(moves, i); 
    }

    return slist_size(moves);
}

int ignore_move(Position* pos, Point pt, Color c) {

    // ignore NOT EMPTY Points

    if (point_color(pos, pt) != EMPTY)
        return 1;

    // ignore true eyes for player

    if (is_eye(pos, pt) == c)
        return 1;

    // ignore statically detected irrelevant moves

    if (is_rejected_in_tree(pos, pt))
        return 1;

    // accept move

    return 0; 
}

Point choose_uniform_random_move(Position* pos, Point i0, int disp)
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
            break;
        }
    not_found:
        i++;
        if (i >= BOARD_IMAX) i = BOARD_IMIN - 1;
    } while (i != i0);
    return move;
}

Point choose_random_move(Position *pos, Point i0, int disp)
// Replace the sequence gen_playout_moves_random(); choose_from()
{
    char   *ret;
    Color c=board_color_to_play(pos);
    Info     sizes[20];
    Point    ds[20], i=i0, move=PASS_MOVE;

    do {
        if (point_color(pos,i) != EMPTY) 
            goto not_found;

        if (is_eye(pos,i) == c) 
            goto not_found;  // ignore true eyes for player

        ret = play_move(pos, i);
        if (ret[0] == 0) {    // move OK
            move = i;
            // check if the suggested move did not turn out to be a self-atari
            int r = random_int(10000), tstrej;
            tstrej = r<=10000.0*PROB_RSAREJECT;
            if (tstrej) {
                slist_clear(ds); slist_clear(sizes);
                fix_atari(pos, i, SINGLEPT_OK, TWOLIBS_TEST, 1, ds, sizes);
                if (slist_size(ds) > 0) {
                    if(disp) fprintf(stderr, "rejecting self-atari move %s\n",
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
    } while (i!=i0);
    return move;
}

Point choose_from(Position *pos, Slist moves, char *kind, int disp)
{
    char   *ret;
    Info   sizes[20];
    Point  move = PASS_MOVE, ds[20];

    FORALL_IN_SLIST(moves, pt) {
        if (is_marked(already_suggested, pt))
            continue;
        mark(already_suggested, pt);
        if (disp && strcmp(kind, "random")!=0)
            fprintf(stderr,"move suggestion (%s) %s\n", kind,str_coord(pt,buf));
        ret = play_move(pos, pt);
        if (ret[0] == 0) {    // move OK
            move = pt;
            // check if the suggested move did not turn out to be a self-atari
            int r = random_int(10000), tstrej;
            if (strcmp(kind,"random") == 0) tstrej = r<=10000.0*PROB_RSAREJECT;
            else                            tstrej = r<= 10000.0*PROB_SSAREJECT;
            if (tstrej) {
                slist_clear(ds); slist_clear(sizes);
                fix_atari(pos, pt, SINGLEPT_OK, TWOLIBS_TEST, 1, ds, sizes);
                if (slist_size(ds) > 0) {
                    if(disp) fprintf(stderr, "rejecting self-atari move %s\n",
                                                           str_coord(pt, buf));
                    undo_move(pos);
                    hobot_assert(pos, blocks_OK(pos,pt));
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

Point choose_capture_move(Position *pos,Slist heuristic_set,float prob,int disp)
// Replace the sequence gen_playout_capture_moves(); choose_from()
{
    int   twolib_edgeonly = 1;
    Point move=PASS_MOVE, moves[20], sizes[20];

    if (random_int(10000) <= prob*10000.0) {
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

double playout_score(Position *pos, int owner_map[], int score_count[2*N*N+1])
// compute score (>0 if BLACK wins); this assumes a final position with all 
// dead stones captured and only single point eyes on the board ...
{
    double s1;
    int s=0;

    FORALL_POINTS(pos,pt) {
        Color c = point_color(pos, pt);
        if (c == EMPTY) c = is_eyeish(pos,pt);
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
    score_count[s + N*N]++;
    return s1 - board_komi(pos) - board_delta_komi(pos);
}

double mcplayout(Position* pos, int amaf_map[], int owner_map[],
    int score_count[2 * N * N + 1], int disp)
{
    // Start a Monte Carlo playout from a given position, return score for to-play
    // player at the starting position; amaf_map is board-sized scratchpad recording
    // who played at a given position first

    if (random_int(10000) <= PROB_UNIFORM_PLAYOUT * 10000.0)
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

        if (capt_score > MERCY_LIMIT
            || capt_score < -MERCY_LIMIT)
        {
            return capt_score;
        }

    }

    s = playout_score(pos, owner_map, score_count);
    return s;
}

double hobot_playout(Position *pos, int amaf_map[], int owner_map[],
                                           int score_count[2*N*N+1], int disp)
// Start a Monte Carlo playout from a given position, return score for to-play
// player at the starting position; amaf_map is board-sized scratchpad 
// recording who played at a given position first
{
    int depth = 0;
    double s=0.0;
    int    passes=0;
    Point  last_moves_neighbors[40], moves[BOARDSIZE], move;
    if(disp) {
        disp_ladder = 1;
        fprintf(stderr, "** SIMULATION **\n");
    }
    if (board_nmoves(pos)>0 && board_last_move(pos)==0) passes=1;

    while (passes < 2 && board_nmoves(pos) < MAX_GAME_LEN) {
        hobot_assert(pos, all_blocks_OK(pos));
        move = 0;
        if(disp) { 
            fprintf(stderr, "mcplayout: idum = %u\n", idum);
            print_pos(pos, stderr, NULL);
        }
        // We simply try the moves our heuristics generate, in a particular
        // order, but not with 100% probability; this is on the border between
        // "rule-based playouts" and "probability distribution playouts".

        make_list_last_moves_neighbors(pos, last_moves_neighbors, 4);

        // 1. Capture heuristic suggestions

        if((move=choose_capture_move(pos, last_moves_neighbors, 
                        PROB_HEURISTIC_CAPTURE, disp)) != PASS_MOVE)
                goto found;

        // [14599] winrate 0.507 | seq  D4 Q4 D16 Q16 R17| can  D4(0.507) Q17(0.484) C4(0.493) Q16(0.495) D3(0.479)


        // 2. Large pattern heuristic suggestions
        // (in the first 1/3 of the simulations
        // and at low depths)

        if (depth < 8 && is_beyond_one_third == 0) 
        {
            if (random_int(10000) <= PROB_HEURISTIC_LARGE_PAT * 10000.0) 
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

        // 3. 3x3 patterns heuristic suggestions

        if (gen_playout_moves_pat3(pos, last_moves_neighbors,
                                           PROB_HEURISTIC_PAT3, moves)) {
            mark_init(already_suggested);
            if((move=choose_from(pos, moves, "pat3", disp)) != PASS_MOVE) {
                mark_release(already_suggested);
                goto found;
            }
            mark_release(already_suggested);
        }
            
        // 4. Random move suggestions

        int x0 = random_int(N) + 1, y0 = random_int(N) + 1;
        move = choose_random_move(pos, y0*(N+1) + x0 , disp);

found:
        depth++;
        if (move == PASS_MOVE) {      // No valid move : pass
            pass_move(pos);
            passes++;
        }
        else 
        {
            if (amaf_map[move] == 0)      // mark the point with 1 for BLACK
                // WHITE because in michi.py pos is updated after this line
                amaf_map[move] = (board_color_to_play(pos) == WHITE ? 1 : -1);
                // TODO: make amaf premium depth-dependent
            passes = 0;
        }

        // mercy break

        double capt_score = get_capture_score(pos);

        if (capt_score > MERCY_LIMIT
        || capt_score < -MERCY_LIMIT) 
        {
            return capt_score;
        }

    }
    s = playout_score(pos, owner_map, score_count);
    return s;
}

int get_capture_score(Position* pos)
{
    return (pos->caps[1] - pos->caps[0] - board_komi(pos) - board_delta_komi(pos));
}

Point random_move_by_coors() {
    int x0 = random_int(N) + 1;
    int y0 = random_int(N) + 1;
    Point pt = y0 * (N + 1) + x0;
    return pt;
}

//========================== Montecarlo tree search ===========================
TreeNode* new_tree_node(void)
// Build a new tree node initialized with prior EVEN
{
    TreeNode *node = hobot_calloc(1,sizeof(TreeNode));
    node->prior_visits = PRIOR_EVEN; node->prior_wins = PRIOR_EVEN/2;
    return node;
}

void expand(Position *pos, TreeNode *tree, int owner_map[])
// add and initialize children to a leaf node which represents the Position pos
{
    char     cfg_map[BOARDSIZE];
    int      nchildren = 0;
    Info     sizes[BOARDSIZE];
    Point    moves[BOARDSIZE];
    Position pos2;
    TreeNode *childset[BOARDSIZE], *node;
    double pattern_scores[BOARDSIZE];

    // Prepare common fate graph map

    if (board_last_move(pos) != PASS_MOVE) 
    {
        compute_cfg_distances(pos, board_last_move(pos), cfg_map);
    }

    // Prepare for large pattern detection

    copy_to_large_board(pos);
    pos2 = *pos;

    // Get all the relevant empty points 
    // (no true enemy eyes, no irrelevant low moves)
    // NOTE: the only usage of this function

    gen_potential_tree_moves(pos, moves, BOARD_IMIN-1);

    tree->children = hobot_calloc(slist_size(moves)+1, sizeof(TreeNode*));

    FORALL_IN_SLIST(moves, pt) 
    {
        assert(point_color(pos, pt) == EMPTY);
        char* ret = play_move(pos, pt);
        
        // Possible failure reasons:
        // move is suicide, move retakes ko. 

        if (ret[0] != 0) 
            continue;

        pattern_scores[pt] = 0.000;
        pattern_scores[pt] = large_pattern_probability(pt, pos->ko);

        undo_move(pos);

#ifdef EXCLUDE_PATTERNS
        // artificially high pattern score = EXCLUSION...

        if (pattern_scores[pt] > 99.0 && pos->ko == 0)
            continue;

        // ...unless we are playing a ko

        else if (pattern_scores[pt] > 99.0)
            pattern_scores[pt] = 0;
#endif

        node = childset[pt] = tree->children[nchildren++] = new_tree_node();
        node->move = pt;
    }

    tree->nchildren = nchildren;

    // Update the prior for the capture

    gen_playout_moves_capture(pos, allpoints, 1, 1, moves, sizes);
    
    int k = 1;
    FORALL_IN_SLIST(moves, pt) 
    {    
        char* ret = play_move(pos, pt);

        if (ret[0] != 0) 
            continue;
        
        undo_move(pos);

        node = childset[pt];
        if (sizes[k] == 1) {
            node->prior_visits += PRIOR_CAPTURE_ONE;
            node->prior_wins += PRIOR_CAPTURE_ONE;
        }
        else {
            node->prior_visits += PRIOR_CAPTURE_MANY;
            node->prior_wins += PRIOR_CAPTURE_MANY;
        }
        k++;
    }

    // Update the prior for the 3x3 patterns suggestions

    gen_playout_moves_pat3(pos, allpoints, 1, moves);
    
    FORALL_IN_SLIST(moves, pt) {
        char* ret = play_move(pos, pt);
        
        if (ret[0] != 0) 
            continue;

        undo_move(pos);
        node = childset[pt];
        node->prior_visits += PRIOR_PAT3;
        node->prior_wins += PRIOR_PAT3;
    }

    double best_pattern = 0.0;

    // Second pass: setting priors based on common fate graph,
    // adjustement for stone height if there are no stones nearby
    // and relevant patterns occuring next to recently played stones

    for (int k=0 ; k<tree->nchildren ; k++) {
        node = tree->children[k];
        Point pt = node->move;
        play_move(&pos2, pt);   // No need to check the move is legal

        if (board_last_move(pos) != PASS_MOVE 
            && cfg_map[pt]-1 < LEN_PRIOR_CFG) {
            node->prior_visits += PRIOR_CFG[cfg_map[pt]-1];
            node->prior_wins += PRIOR_CFG[cfg_map[pt]-1];
        }

        int height = line_height(pt, board_size(pos));  // 0-indexed

        if (height <= 4 && empty_area(pos, pt, 3)) {

            // No stones around; negative prior for the 1st + 2nd lines, 
            // positive for the 3rd line and smaller positive prior 
            // for the 4th line. 5th line is again discouraged. 
            // It sanitizes opening and invasions.

            if (height <= 1) {
                node->prior_visits += PRIOR_EMPTYAREA;
                node->prior_wins += 0;
            }
            else if (height == 2) {
                node->prior_visits += PRIOR_EMPTYAREA;
                node->prior_wins += PRIOR_EMPTYAREA;
            }
            else if (height == 3) {
                node->prior_visits += PRIOR_EMPTYAREA / 2;
                node->prior_wins += PRIOR_EMPTYAREA / 2;
            }
            else if (height == 4) {
                node->prior_visits += PRIOR_EMPTYAREA;
                node->prior_wins += 0;
            }
        }
        
        // Assign pattern score to vertices 
        // included in common fate graph map

        if (board_last_move(pos) != PASS_MOVE
        && cfg_map[pt] - 1 < LEN_PRIOR_CFG) 
        {
            double patternprob = pattern_scores[pt];

            // Get max probability of a cfg-relevant pattern.
            // If best_pattern will turn out higher than threshold,
            // we will not generate patterns for the rest 
            // of the board. This attempts to capture the notion 
            // of an urgent move.
            
            if (patternprob > best_pattern)
                best_pattern = patternprob;

            if (patternprob > 0.0) 
            {
                double pattern_prior = sqrt(patternprob); // tone up

                node->prior_visits += pattern_prior * PRIOR_LARGEPATTERN;
                node->prior_wins += pattern_prior * PRIOR_LARGEPATTERN;
            }
        }

        undo_move(&pos2);

        // preference for contested areas

        if (global_simulation_count > 2000) {
            double ownership = 100 * owner_map[pt] / global_simulation_count;

            //sprintf(buf, "map %d cnt %d product %5.5f", owner_map[pt], global_nodecount, ownership);
            //log_fmt_s('S', buf, NULL);

            if (ownership > -10.0 && ownership < 10.0) {
                node->prior_visits += PRIOR_OWNER;
                node->prior_wins += PRIOR_OWNER;
            }

            if (ownership > -20.0 && ownership < 20.0) {
                node->prior_visits += PRIOR_OWNER;
                node->prior_wins += PRIOR_OWNER;
            }

            if (ownership > -30.0 && ownership < 30.0) {
                node->prior_visits += PRIOR_OWNER;
                node->prior_wins += PRIOR_OWNER;
            }

            if (ownership > -40.0 && ownership < 40.0) {
                node->prior_visits += PRIOR_OWNER;
                node->prior_wins += PRIOR_OWNER;
            }

            if (ownership < -60.0 && ownership > 60.0) {
                node->prior_visits += PRIOR_OWNER;
                node->prior_wins += 0;
            }

            if (ownership < -70.0 && ownership > 70.0) {
                node->prior_visits += PRIOR_OWNER;
                node->prior_wins += 0;
            }

            if (ownership < -80.0 && ownership > 80.0) {
                node->prior_visits += PRIOR_OWNER;
                node->prior_wins += 0;
            }
        }
    }

    // Third pass: if we didn't find an urgent local move, 
    // we need to initialize patterns globally; if we did,
    // only urgent moves get pattern bonus.

    if (best_pattern < URGENT_PATTERN_SCORE) 
    {
        for (int k = 0; k < tree->nchildren; k++) {

            node = tree->children[k];
            Point pt = node->move;
            play_move(&pos2, pt);   // No need to check the move is legal

            double patternprob = pattern_scores[pt];

            if (patternprob > 0.0
            && !((board_last_move(pos) != PASS_MOVE
                && cfg_map[pt] - 1 < LEN_PRIOR_CFG)) ) {
                double pattern_prior = sqrt(patternprob);       // tone up

                node->prior_visits += pattern_prior * PRIOR_LARGEPATTERN;
                node->prior_wins += pattern_prior * PRIOR_LARGEPATTERN;
            }
            undo_move(&pos2);
        }
    } else {
        for (int k = 0; k < tree->nchildren; k++) {

            node = tree->children[k];
            Point pt = node->move;
            play_move(&pos2, pt);   // No need to check the move is legal

            double patternprob = pattern_scores[pt];

            // some patterns can be encouraged
            // even as a tenuki from an urgent move

            if (patternprob >= TENUKI_PATTERN_SCORE  
                && !((board_last_move(pos) != PASS_MOVE
                && cfg_map[pt] - 1 < LEN_PRIOR_CFG))) {
                
                double pattern_prior = sqrt(patternprob);       // tone up

                node->prior_visits += pattern_prior * PRIOR_LARGEPATTERN;
                node->prior_wins += pattern_prior * PRIOR_LARGEPATTERN;
            }
            undo_move(&pos2);
        }

    }

    // Fourth pass: rudimentary tactical analysis.
    // Detecting self-atari, moves caught in a ladder,
    // moves caught in a simple net and more two-liberties tactics.

    for (int k = 0; k < tree->nchildren; k++) {

        node = tree->children[k];
        Point pt = node->move;
        play_move(&pos2, pt);

        int in_atari = fix_atari(&pos2, pt, SINGLEPT_OK, TWOLIBS_TEST,
            !TWOLIBS_EDGE_ONLY, moves, sizes);

        
        if (in_atari == 0) {
            int cnt = count_atari(&pos2, pt);

            // single atari

            if (cnt == 1) {
                node->prior_visits += PRIOR_ATARI;
                node->prior_wins += PRIOR_ATARI;
            }

            // double atari

            if (cnt > 1) {
                node->prior_visits += PRIOR_DBL_ATARI;
                node->prior_wins += PRIOR_DBL_ATARI;
            }
        }

        if (slist_size(moves) > 0) {
            node->prior_visits += PRIOR_SELFATARI;
            node->prior_wins += 0;  // negative prior
            node->visits += 5; // cheating: self-atari needs more effort

            // ladder fix

            char* escapes = slist_str_as_point(moves); // list of atari escapes
            int l = (int)strlen(escapes);

            if (in_atari == 0 && l > 0) {
                Block bb = point_block(&pos2, pt);

                if (block_nlibs(&pos2, bb) == 2) {
                    node->prior_visits += 100;
                    node->prior_wins /= 4;  // negative prior + decreasing wins
                }
            }
        }
        else {
            int min_libs = get_min_libs(pos, pt);
            if (min_libs < 10) {
                node->prior_visits += lib_shortage_bonus[min_libs];
                node->prior_wins += lib_shortage_bonus[min_libs];
            }
        }
        undo_move(&pos2);
    }

    // Fifth step (optional): add a pass move.
    // Useful, for example in case of seki

    if (tree->nchildren <= 2) 
    {
        int nc = tree->nchildren;    
        tree->children[nc] = new_tree_node();
        tree->children[nc]->move = PASS_MOVE;
        tree->nchildren = nc+1;
    }
}

void free_tree(TreeNode *tree)
// Free memory allocated for the tree
{
    if (tree->children != NULL) {
        for (TreeNode **child = tree->children ; *child != NULL ; child++)
                free_tree(*child);
        free(tree->children);
    }
    free(tree);
}

double rave_urgency(TreeNode *node)
{
    double v = node->visits + node->prior_visits;
    double expectation = (node->wins + node->prior_wins)/v;
    if (node->amaf_visits==0) return expectation;

    double rave_expectation = (double) node->amaf_wins / (double) node->amaf_visits;
    double beta = node->amaf_visits / (node->amaf_visits + v + (double)v*node->amaf_visits/RAVE_EQUIV);
    return beta * rave_expectation + (1-beta) * expectation;
}

double winrate(TreeNode *node)
// Return the winrate (number of wins divided by the number of visits)
{
    double wr;
    if (node->visits>0) wr = (double) node->wins / (double) node->visits;
    else           wr = -0.1;
    return wr;
}

void find_two_most_visited_children(TreeNode *tree, TreeNode **bests, int *v)
// find the two children that have been most visited
{
    int vmax=-1, vmax2=-1;
    TreeNode *c=NULL, *c2=NULL;

    if (tree->children != NULL) {
        for (TreeNode **child = tree->children ; *child != NULL ; child++) {
            if ((*child)->visits > vmax2) {
                if ((*child)->visits > vmax) {
                    vmax2 = vmax;
                    c2 = c;
                    vmax = (*child)->visits;
                    c = *child;
                }
                else {
                    vmax2 = (*child)->visits;
                    c2 = *child;
                }
            }
        }
    }
    bests[0] = c;
    bests[1] = c2;
    v[0] = vmax;
    v[1] = vmax2; 
}

TreeNode* best_move(TreeNode *tree, TreeNode **except)
// best move is the most simulated one (avoiding nodes in except list)
{
    int vmax=-1;
    TreeNode *best=NULL;

    if (tree->children == NULL) 
        return NULL;

    for (TreeNode **child = tree->children ; *child != NULL ; child++) {
        if ((*child)->visits > vmax) {
            int update = 1;
            if (except != NULL) 
                for (TreeNode **n=except ; *n!=NULL ; n++)
                    if (*child == *n) update=0;
            if (update) {
                vmax = (*child)->visits;
                best = (*child);
            }
        }
    }
    return best;
}

void dump_subtree(TreeNode *node, double thres, char *indent, FILE *f
                                                                , int recurse);
TreeNode* most_urgent(TreeNode **children, int nchildren, int disp)
// Return the most urgent child to play
{
    int    k=0;
    double urgency, umax=0;
    TreeNode *urgent = children[0];

    // Randomize the order of the nodes
    SHUFFLE(TreeNode *, children, nchildren);

    for (TreeNode **child = children ; *child != NULL ; child++) {
        if (disp)
            dump_subtree(*child, N_SIMS/50, "", stderr, 0);
        urgency = rave_urgency(*child);
        if (urgency > umax) {
            umax = urgency;
            urgent = *child;
        }
        k++;
    }
    return urgent;
}

int tree_descend(Position *pos, TreeNode *tree, int amaf_map[], int owner_map[], int disp
                                                            , TreeNode **nodes)
// Descend through the tree to a leaf
{
    int last=0, passes = 0;
    Point move;
    //tree->v += 1;
    nodes[last] = tree;
   
    while (nodes[last]->children != NULL && passes <2) {
        
        if (disp) 
            print_pos(pos, stderr, NULL);
        
        // Pick the most urgent child
        TreeNode *node = most_urgent(nodes[last]->children, 
                                            nodes[last]->nchildren, disp);
        nodes[++last] = node;
        move = node->move;
        if (disp) { fprintf(stderr, "chosen "); ppoint(move); }

        if (move == PASS_MOVE) {
            passes++;
            pass_move(pos);
        }
        else {
            passes = 0;
            play_move(pos, move);
            if (amaf_map[move] == 0) //Mark the point with 1 for black
                amaf_map[move] = (board_color_to_play(pos) == BLACK ? -1 : 1);
        }

        if (node->children == NULL && node->visits >= EXPAND_VISITS)
            expand(pos, node, owner_map);
    }
    return last;
}

void tree_update(Position *pos, TreeNode **nodes, int last
                                    , int amaf_map[], double score, int disp)
// Store simulation result in the tree (nodes is the tree path)
{
    int amaf_map_value = 1;
    if (board_color_to_play(pos) == WHITE) {
        amaf_map_value = -1;
        score = -score;  // score > 0 means the player in the first node wins
    }
    for (int k=0 ; k<=last ; k++) {
        TreeNode *n= nodes[k];
        if(disp) {
            char str[8]; 
            str_coord(n->move, str);
            fprintf(stderr, "updating %s %d\n", str, score<0.0); 
        }
        n->visits += 1;         // TODO put it in tree_descend when parallelize
        n->wins += score<0.0;  // score is for to-play, node stats for just-played
        
        // Update the node children AMAF stats with moves we made 
        // with their color
        if (n->children != NULL) {
            for (TreeNode **child = n->children ; *child != NULL ; child++) {
                if ((*child)->move == 0) continue;
                if (amaf_map[(*child)->move] == amaf_map_value) {
                    if (disp) {
                        char str[8];
                        str_coord((*child)->move, str);
                        fprintf(stderr, "  AMAF updating %s %d\n", str,score>0);
                    }
                    (*child)->amaf_wins += score > 0; // reversed perspective
                    (*child)->amaf_visits += 1;
                }
            }
        }
        score = -score;
        amaf_map_value = -amaf_map_value;
    }
}

float nplayouts, nplayouts_per_second=-1.0;
float start_playouts_sec, stop_playouts_sec;
int   nplayouts_real;

void update_speed()
// Update the number of playouts per seconds
{
    stop_playouts_sec=(float) clock() / (float) CLOCKS_PER_SEC;
    nplayouts_per_second = nplayouts_real / 
                            (stop_playouts_sec-start_playouts_sec); 
}

float best2, bestr, bestwr;
void collect_infos(TreeNode *tree, int n, TreeNode *best,
        TreeNode *workspace[], Position *pos)
{
    char str[10], str2[10], str_r[10];
    TreeNode *second, *reply;

    nplayouts_real += n;
    update_speed();

    workspace[0] = best; 
    workspace[1] = 0;
    second = best_move(tree, workspace);
    bestwr= winrate(best);
    if (second != NULL) {
        best2 = bestwr / winrate(second);
        str_coord(second->move, str2);
    }
    else {
        best2 = 0.0;
        str2[0] = 0;
    }
    reply = best_move(best, NULL);
    if (reply != NULL) {
        bestr = bestwr - winrate(reply);
        str_coord(reply->move, str_r);
    }
    else {
        bestr = 0.0;
        str_r[0] = 0;
    }
    str_coord(best->move, str);
    sprintf(buf, "%12u %5.1f %3s (%3s) %3s %5.2f %6.3f %6.3f %7.1f"
               , idum, board_delta_komi(pos), str, str2, str_r
               , best2, bestr, bestwr, nplayouts_per_second);
    log_fmt_s('S', buf, NULL);
}

void print_tree_summary(TreeNode *tree, int sims, FILE *f);

Point tree_search(Position *pos, TreeNode *tree, int n, int owner_map[],
                                              int score_count[] , int disp)
// Perform MCTS search from a given position for a given number of iterations
{
    double s;
    int *amaf_map=hobot_calloc(BOARDSIZE, sizeof(int)), i, last, visit[2]; 
    Point    bestmove;
    Position *workpos=hobot_malloc(sizeof(Position));
    TreeNode *best, *bests[2], *nodes[500];

    int start_time = Now();
    is_beyond_halfway = 0;
    is_beyond_one_third = 0;

    pos->delta_komi = 0;

    // Initialize the root node if necessary
    if (tree->children == NULL) 
        expand(pos, tree, owner_map);

    // Test mode to view priors quality
    if (N_SIMS == 0)
        return get_best_prior(pos, tree);

    int live_gfx = strcmp(Live_gfx,"None") != 0;

    for ( i=0; i<n ; i++) {

        // TODO: is_early, is_late
        global_simulation_count = i;

        if (i > n / 2)
            is_beyond_halfway = 1;
        if (i > n / 3)
            is_beyond_one_third = 1;
        
        if (live_gfx && (i % Live_gfx_interval) == Live_gfx_interval-1)
            display_live_gfx(pos, tree, owner_map);

        *workpos = *pos;
        memset(amaf_map, 0, BOARDSIZE*sizeof(int));
        
        if (i>0 && i % REPORT_PERIOD == 0) 
            print_tree_summary(tree, i, stderr); 

        last = tree_descend(workpos, tree, amaf_map, owner_map, disp, nodes);
        s = mcplayout(workpos, amaf_map, owner_map, score_count, disp);
        tree_update(pos, nodes, last, amaf_map, s, disp);
        best = best_move(tree, NULL);
        double best_wr = winrate(best);

        // check for timeout

        if (i % 513 == 0) {
            int now = Now();
            if (now - start_time > MAX_TIME)
                break;
        }

        // adjust dynamic komi 
        // (increase difficulty while winning handily)

        if (is_beyond_halfway && (i % DYNKOMI_PERIOD == 0))
            adjust_komi(pos, best_wr);

        // try to shorten search when the second best move
        // cannot beat the best one

        if (is_beyond_halfway) {
            find_two_most_visited_children(tree, bests, visit);
            if (visit[1] + n - i < visit[0]) {
                saved_nodes += (n - i);
                sprintf(buf, "early abort save %d / %d simulations", n - i, n);
                log_fmt_s('S', buf, NULL);
                break;          // 2nd most visited child will not be able to win
            }
        }
    }

    best = best_move(tree, NULL);
    collect_infos(tree, i, best, nodes, pos);

finished:
    if (verbosity > 0) {
        dump_subtree(tree, N_SIMS/50, "", stderr, 1);
        print_tree_summary(tree, i, stderr);
    }

    free(amaf_map); free(workpos);
    if (best->move == PASS_MOVE && board_last_move(pos) == PASS_MOVE)
        bestmove = PASS_MOVE;

    else if (((double) best->wins / (double) best->visits) < RESIGN_THRES) 
        bestmove = RESIGN_MOVE;
    else
        bestmove = best->move;
    return bestmove;
}

Point get_best_prior(Position* pos, TreeNode* tree)
{
    char str[12], str2[20];
    double dvmax;
    int   vmax = 0;
    Point best_move = PASS_MOVE;
    TreeNode* n;

    for (TreeNode** child = tree->children; *child != NULL; child++) {
        n = *child;
        if (n->prior_wins > vmax) {
            vmax = n->prior_wins;
            best_move = n->move;
        }

        // safeguard
        if (best_move == PASS_MOVE
            && point_color(pos, n->move) == EMPTY)
            best_move = n->move;
    }
    dvmax = vmax;

    for (TreeNode** child = tree->children; *child != NULL; child++) {
        n = *child;
        Point pt = n->move;
        double p = n->prior_wins / dvmax;
        p = round(sqrt(p) * 10.0) / 10.0;
        str_coord(pt, str);
        strcat(buf, str2);
    }

    return best_move;
}


int Now(void) {

#if defined(_WIN32) || defined(_WIN64)
    return GetTickCount(); // bugbug:drc GetTickCount() wraps once every 50 days, causeing time control to go insane.  Don't use this.
#else
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}


//-------------------------- Print of the MCTS tree ---------------------------
void dump_subtree(TreeNode *node, double thres, char *indent, FILE *f
                                                                , int recurse)
// print this node and all its children with v >= thres.
{
    char str[8], str_winrate[8], str_rave_winrate[8];
    str_coord(node->move, str);
    if (node->visits) sprintf(str_winrate, "%.3f", winrate(node)); 
    else         sprintf(str_winrate, "nan");
    if (node->amaf_visits) sprintf(str_rave_winrate, "%.3f", (double)node->amaf_wins/node->amaf_visits); 
    else          sprintf(str_rave_winrate, "nan");
    fprintf(f,"%s+- %s %5s "
              "(%6d/%-6d, prior %3d/%-3d, rave %6d/%-6d=%5s, urgency %.3f)\n"
           ,indent, str, str_winrate, node->wins, node->visits, node->prior_wins, node->prior_visits
           ,node->amaf_wins, node->amaf_visits, str_rave_winrate, rave_urgency(node));
    if (recurse) {
        char new_indent[BUFLEN];
        sprintf(new_indent,"%s   ", indent);
        for (TreeNode **child = node->children ; *child != NULL ; child++)
            if ((*child)->visits >= thres)
                dump_subtree((*child), thres, new_indent, f, 0);
    }
}

void dump_node(TreeNode *node, FILE *f)
// Dump node
{
    char str[8];
    int k=1;
    fprintf(f,"child move   winrate      prior       rave      urgency nch\n");
    fprintf(f,"self  %4s %5d/%-5d %5d/%-5d %5d/%-5d %8.5lf %3d\n"
            , str_coord(node->move,str)
            , node->wins, node->visits, node->prior_wins,node->prior_visits, node->amaf_wins,node->amaf_visits
            , rave_urgency(node), node->nchildren);
    if (node->nchildren) {
        for (TreeNode **c = node->children ; *c != NULL ; c++) {
            TreeNode*n = *c;
            fprintf(f,"%5d %4s %5d/%-5d %5d/%-5d %5d/%-5d %8.5lf %3d\n"
                    , k++, str_coord(n->move,str)
                    , n->wins, n->visits, n->prior_wins, n->prior_visits, n->amaf_wins, n->amaf_visits
                    , rave_urgency(n), n->nchildren);
        }
    }
}

void compute_principal_variation(TreeNode *tree, Point moves[6])
// Return the best sequence of moves (according to the tree)
{
    TreeNode *node = tree;
    slist_clear(moves);
    for (int k=0 ; k<5 ; k++) {
        node = best_move(node, NULL);
        if (node == NULL) break;
        slist_push(moves, node->move);
    }
}

void compute_best_moves(TreeNode *tree, char sep[2]
                                        , TreeNode *best_node[5], char *can)
// Build a string with the coordinates and winrate of the 5 best moves
{
    char str[8], tmp[32];

    can[0] = 0;

    for (int k=0 ; k<5 ; k++) { 
        best_node[k] = best_move(tree, best_node);
        if (best_node[k] != NULL) {
            str_coord(best_node[k]->move, str);
            if (best_node[k]->visits)
                sprintf(tmp, " %s%c%.3f%c", str
                        , sep[0], winrate(best_node[k]), sep[1]);
            else
                sprintf(tmp, " %s%c%s%c", str, sep[0], "nan", sep[1]);
            strcat(can, tmp);
        }
    }
}

void print_tree_summary(TreeNode *tree, int sims, FILE *f)
{
    char can[128];
    Point moves[6];
    TreeNode *best_node[5]={0,0,0,0,0};

    compute_principal_variation(tree, moves);
    compute_best_moves(tree, "()", best_node, can);

    fprintf(f,"[%4d] winrate %.3f | seq %s| can %s\n", sims,
            winrate(best_node[0]), slist_str_as_point(moves), can);   
}

//--------------------------- Print of the Position ---------------------------
void make_pretty(Position *pos, char pretty_board[BOARDSIZE], int *capB
                                                                , int *capW);

void print_pos(Position *pos, FILE *f, int *owner_map)
// Print visualization of the given board position
{
    char pretty_board[BOARDSIZE], strko[8];
    int  capB, capW;

    make_pretty(pos, pretty_board, &capB, &capW);
    fprintf(f,"Move: %-3d   Black: %d caps   White: %d caps   Komi: %.1f",
            board_nmoves(pos), capB, capW, board_komi(pos)); 
    if (board_ko(pos))
        fprintf(f,"   ko: %s", str_coord(board_ko(pos), strko)); 
    fprintf(f,"\n");

    int size = board_size(pos), skipped=N-size+1;
    int k=skipped*(N+1), k1=skipped*(N+1);
    for (int row=skipped ; row<=N ; row++) {
        if (board_last_move(pos) == k+1) fprintf(f, " %-2d(", N-row+1); 
        else                             fprintf(f, " %-2d ", N-row+1);
        k++; k1++;
        for (int col=1 ; col<=size ; col++, k++) {
            fprintf(f, "%c", pretty_board[k]);
            if (board_last_move(pos) == k+1)    fprintf(f, "(");
            else if (board_last_move(pos) == k) fprintf(f, ")");
            else                                fprintf(f, " ");
        }
        if (owner_map) {
            fprintf(f, "   ");
            for (int col=1 ; col<=size ; col++, k1++) {
                char c;
                if      ((double)owner_map[k1] > 0.6*nplayouts_real) c = 'X';
                else if ((double)owner_map[k1] > 0.3*nplayouts_real) c = 'x';
                else if ((double)owner_map[k1] <-0.6*nplayouts_real) c = 'O';
                else if ((double)owner_map[k1] <-0.3*nplayouts_real) c = 'o';
                else                                                 c = '.';
                fprintf(f, " %c", c);
            }
        }
        k += skipped-1; k1 += skipped-1;
        fprintf(f, "\n");
    }
    fprintf(f, "    ");
    for (int col=1 ; col<=size ; col++) fprintf(f, "%c ", colstr[col]);
    fprintf(f, "\n\n");
}
