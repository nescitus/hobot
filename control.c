// control.c -- Overall management of the MCTS search (dynkomi, time management)
#include "hobot.h"

float saved_time_left;
#define max(a, b) ((a)<(b) ? (a) : (b))
int saved_nodes;

//----------------------------- time management -------------------------------
int nsims(Game *game) 
// Compute the number of simulations for the next tree_search.
// Simplistic time management (portable if not very accurate).
// Loosely based on Petr Baudis Thesis.
{
    int nstones, moves_to_play;

    if (game->time_init <= 0) {
        start_playouts_sec = (float) clock() / (float) CLOCKS_PER_SEC;
        return N_SIMS;     // Game is not time limited
    }

    if (nplayouts_per_second <= 0.0) {
        start_playouts_sec = (float) clock() / (float) CLOCKS_PER_SEC;
        nplayouts = N_SIMS;
        return N_SIMS;     // 1st genmove
    }

    // Game is time limited and we know playout speed: nplayouts_per_second
    saved_time_left -= stop_playouts_sec - start_playouts_sec; 
    if (game->time_left_gtp > 0)
        game->time_left = game->time_left_gtp;
    else
        game->time_left = saved_time_left;

    sprintf(buf,"time used:      %6d %7.1f", 
            nplayouts_real, stop_playouts_sec-start_playouts_sec);
    log_fmt_s('T', buf, NULL);
    start_playouts_sec = (float) clock() / (float) CLOCKS_PER_SEC;

    nstones = board_nstones(game->pos);
    moves_to_play = ((N*N*3)/4 - nstones)/2;// Hyp: 25 % of EMPTY points at end
    // Most often the thinking time will be doubled in genmove
    // We anticipate that fact here in order to allocate most of the thinking
    // time for the middle game
    float reduction = 1.0;
    if (moves_to_play < 30) {
        moves_to_play = 30;
        reduction = 2.0;
    }
    if (board_nmoves(game->pos) < 15) 
        reduction = 2.0;
    nplayouts = (float) game->time_left / moves_to_play  * nplayouts_per_second;
    nplayouts /= reduction;
    if (nplayouts > 100000) nplayouts = 100000;

    sprintf(buf,"time allocated: %6.0f %7.2f %7.1f %5d %5d",
            nplayouts, nplayouts / nplayouts_per_second,
            nplayouts_per_second, moves_to_play, game->time_left);
    log_fmt_s('T', buf, NULL);
    nplayouts_real = 0;
    return (int) nplayouts;
}

// ----------------------- Is the Position clear enough ? ---------------------
int is_position_clear_enough(void)
// Check if the situation is clear enough after the first tree search.
{
    if (bestwr < 0.48)                             // program is behind
        //|| best2 < 2.0  || fabs(bestr) > 0.02)   // unclear situation
        return 0;         // we will try to extend the thinking time
    else
        return 1;
}

// ------ Status of Blocks and Points and computation of  the final score -----
// Borrowed from an old version of Pachi
Status mcts_point_status(Point pt, int owner_map[BOARDSIZE])
// Status of points: OWN_BY_BLACK, OWN_BY_WHITE or UNKNOWN
{
    double nsims=nplayouts_real;
    Status p_status;

    if ((double)owner_map[pt] > 0.8*nsims)
        p_status=OWN_BY_BLACK;
    else if ((double)owner_map[pt] < -0.8*nsims)
        p_status=OWN_BY_WHITE;
    else
        p_status=UNKNOWN;

    return p_status;
}

void compute_all_status(Position *pos, int owner_map[BOARDSIZE],
        int score_count[], Status block_status[], Status point_status[])
// Compute status of of points and blocks (based on owner_map)
// - the points : OWN_BY_BLACK, OWN_BY_WHITE or UNKNOWN
// - the blocks : DEAD, ALIVE or UNKNOWN
{
    Color c;
    int   b;
    Status new_g_st, p_st;
    TreeNode *tree=new_tree_node();

    // Launch a full depth MCTS search in order to compute owner_map

    memset(owner_map, 0, BOARDSIZE*sizeof(int));
    nplayouts_real = 0;
    tree_search(pos, tree, 2*N_SIMS, owner_map, score_count, 0);

    sprintf(buf, "nsims: %d", nplayouts_real);
    log_fmt_s('I',buf,NULL);

    // Reset status of points and blocks
    FORALL_POINTS(pos, pt)
        point_status[pt] = UNKNOWN;
    for (b=1 ; b<MAX_BLOCKS ; b++)
        block_status[b] = UNKNOWN;

    // Try to evaluate block status
    FORALL_POINTS(pos, pt) {
        b = point_block(pos, pt);
        if (b != 0) {
            c = point_color(pos, pt);
            point_status[pt] = p_st = mcts_point_status(pt, owner_map);
            if (p_st == UNKNOWN)
                block_status[b] = UNKNOWN;
            else {
                new_g_st = UNKNOWN;
                if ((p_st == OWN_BY_WHITE && c==BLACK) ||
                        (p_st == OWN_BY_BLACK && c==WHITE))
                    new_g_st = DEAD;
                else if ((p_st == OWN_BY_BLACK && c==BLACK) ||
                            (p_st == OWN_BY_WHITE && c == WHITE))
                    new_g_st = ALIVE;

                if(block_status[b] == UNKNOWN)
                    block_status[b] = new_g_st;
                else if (block_status[b] != new_g_st)
                    block_status[b] = UNKNOWN;
            }
        }
        else
            point_status[pt] = mcts_point_status(pt, owner_map);
    }
    free_tree(tree);
}

Score final_score(Game *game, Status block_status[MAX_BLOCKS], 
                                          Status point_status[BOARDSIZE])
// Compute the score of a finished game: score > 0.0 if BLACK wins
// Use approximate area scoring (chinese rules)
{
    double score=0.0, unknown_empty=0.0, unknown_stones=0.0;
    Position *pos=game->pos;
    Score s;

    FORALL_POINTS(pos, pt) {
        Block b = point_block(pos, pt);
        Color c = point_color(pos, pt);
        if (c == BLACK) {
            if (block_status[b] == ALIVE)
                score += 1.0;
            else if (block_status[b] == DEAD)
                score -= 1.0;
            else if (block_status[b] == UNKNOWN)
                unknown_stones += 1.0;
        }
        else if (c == WHITE) {
            if (block_status[b] == ALIVE)
                score -= 1.0;
            else if (block_status[b] == DEAD)
                score += 1.0;
            else if (block_status[b] == UNKNOWN)
                unknown_stones += 1.0;
        }
        else if (c == EMPTY) {
            if (point_status[pt] == OWN_BY_BLACK)
                score += 1.0;
            else if (point_status[pt] == OWN_BY_WHITE)
                score -= 1.0;
            else if (point_status[pt] == UNKNOWN)
                unknown_empty += 1.0;
        }
    }

    score -= board_komi(pos);
    s.raw = score;
    // optimistic score consider 
    // * the stones with unknown status
    //   - as ALIVE if they belong to the computer
    //   - as DEAD if they blong to the opponent
    // * the points with unknown status as belonging to the computer
    // pessimistic score is the opposite
    if (game->computer_color == BLACK) {
        s.optimistic = score + unknown_stones + unknown_empty;
        s.pessimistic = score - unknown_stones - unknown_empty; 
    }
    else {
        s.optimistic = score - unknown_stones - unknown_empty;
        s.pessimistic = score + unknown_stones + unknown_empty; 
    }
    sprintf(buf,"unknown points: %.0lf, scores: %.1lf, %.1lf, %.1lf",
            unknown_stones + unknown_empty, s.pessimistic, s.raw, s.optimistic);
    log_fmt_s('S', buf, NULL);
    return s;
}

// ---------------------------- "Intelligent" passing -------------------------
int is_better_to_pass(Game *game, int *owner_map, int *score_count)
// Check if the computer should pass
{
    int answer, sure_loss, sure_win;
    Position *pos=game->pos;
    Score score;
    Status block_status[BOARDSIZE], point_status[BOARDSIZE];

    if (!play_until_the_end 
        && board_last_move(pos) == PASS_MOVE && board_nmoves(pos)>2) {

        compute_all_status(pos, owner_map, score_count
                                                , block_status, point_status);
        score = final_score(game, block_status, point_status);
        if (score.optimistic - score.pessimistic < 21.0) {
            // Relatively "little" uncertainty
            if (game->computer_color == BLACK) {
                sure_win = score.pessimistic > 0.0;
                sure_loss = score.optimistic < 0.0;
            }
            else {
                sure_win = score.pessimistic < 0.0;
                sure_loss = score.optimistic > 0.0;
            }
        }
        else {
            // Large uncertainty
            if (game->computer_color == BLACK) {
                sure_win = score.pessimistic > -0.9;
                sure_loss = score.optimistic < 1.9;
            }
            else {
                sure_win = score.pessimistic < 0.9;
                sure_loss = score.optimistic > -1.9;
            }
        }
        if (sure_win)
            log_fmt_s('I', "Opponent pass and I estimate that I win", NULL);
        else if (sure_loss)
            log_fmt_s('I', "Opponent pass and I estimate that I loose", NULL);
        else
            log_fmt_s('I', 
                "Opponent pass but I estimate that game is not decided", NULL);
        answer = sure_win || sure_loss;
    }
    else
        answer = 0;
    return answer;
}

// ------------------------- Generate next move -------------------------------
Point genmove (Game *game, TreeNode **tree, int *owner_map, int *score_count)
{
    Point pt;
    Position *pos = game->pos;
    if (board_nstones(pos) <= 9 /* || is_time_limited*/)
        saved_nodes = 0;

    // fuseki on 19x19 board

    if (board_size(pos) == 19) {
        int idum_save = idum;
        idum = true_random_seed();
        pt = get_fuseki_move(pos);
        idum = idum_save;
        if (pt != PASS_MOVE)
            return pt;
    }

    if (is_better_to_pass(game, owner_map, score_count)) {
        pt = PASS_MOVE;
    }

    else {
        free_tree(*tree);
        *tree = new_tree_node();

        log_fmt_s('S', buf, NULL);
        int n = nsims(game);

        //if (!is_time_limited) 
        {
            int addon = saved_nodes / 5;
            sprintf(buf, "extending nodes budget by %d of %d", addon, saved_nodes);
            log_fmt_s('T', buf, NULL);
            n += addon;
            saved_nodes -= addon;
            sprintf(buf, "saved nodes budget is now %d", saved_nodes);
            log_fmt_s('T', buf, NULL);

        }

        memset(owner_map, 0, BOARDSIZE*sizeof(int));
        memset(score_count, 0, (2*N*N+1)*sizeof(int));
        if (use_dynamic_komi)
            update_dyn_komi(game);
        nplayouts_real = 0;
        pt = tree_search(pos, *tree, n, owner_map, score_count, 0);

        if (is_time_limited(game)
            && (nplayouts_real*2 > nplayouts) 
            && !is_position_clear_enough()) {
            // think harder hoping we will recover
            log_fmt_s('S', "thinking time extended", NULL);
            pt = tree_search(pos, *tree, n, owner_map, score_count, 0);
        }
    }
    return pt;
}
