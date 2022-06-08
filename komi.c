// komi.c -- dynamic komi manipulations
#include "hobot.h"

// -------------------------------- Dynamic komi ------------------------------
void update_dyn_komi(Game *game)
// This a simplistic approach used only for handicap games
{
    double dkm, handi=game->handicap, nstones, moves_to_play, endmove;
    Position *pos=game->pos;

    nstones = board_nstones(pos);
    moves_to_play = ((N*N*3)/4 - nstones)/2;// Hyp: 25 % of EMPTY points at end
    endmove = board_nmoves(pos) + moves_to_play; 
    dkm = handi*komi_per_handicap_stone*moves_to_play/endmove;
    if (dkm < 0.0) 
        dkm = 0.0;
    if (board_color_to_play(pos) == WHITE)
        dkm = -dkm;
    board_set_delta_komi(pos, dkm);
}

    void adjust_komi(Position *pos, double winrate)
    // Dead simple dynamic komi. If we win  in more than 80% of simulations,
    // we artificially drive the score down, typically getting a few points'
    // winning margin.
    {
        if (winrate < 0.80) {
            pos->delta_komi = 0;
            return;
        }

        int nstones = board_nstones(pos);
        int moves_to_play = ((N * N * 3) / 4 - nstones) / 2;// Hyp: 25 % of EMPTY points at end
        int endmove = board_nmoves(pos) + moves_to_play;
        double add = (1.0 * moves_to_play) / endmove;
        if (add < 0.0) add = 0.0;

        if (winrate > 0.80) {
            if (pos->to_play == WHITE)
                pos->delta_komi -= add;
            else
                pos->delta_komi += add;
        }
    }