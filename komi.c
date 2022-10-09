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
    {
        // play with noral komi

        if (winrate < DYNKOMI_GREEN) 
        {
            pos->delta_komi = 0;
            return;
        }

        // unused

        int nstones = board_nstones(pos);
        int moves_to_play = ((N * N * 3) / 4 - nstones) / 2;// Hyp: 25 % of EMPTY points at end
        int endmove = board_nmoves(pos) + moves_to_play;
        double add = (1.0 * moves_to_play) / endmove;
        if (add < 0.0) add = 0.0;

        add = 0.5; // testing

        if (winrate > DYNKOMI_GREEN) 
        {
            if (pos->to_play == WHITE)
                pos->delta_komi -= add;
            else
                pos->delta_komi += add;
        }

        // limit komi adjustement

        if (pos->delta_komi < -15)
            pos->delta_komi = -15;
        if (pos->delta_komi > 15)
            pos->delta_komi = 15;
    }