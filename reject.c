// reject.c -- logic for rejecting moves

#include "hobot.h"

int is_rejected_in_tree(Position* pos, Point pt) 
{
    return is_rejected_by_height(pos, pt);
}

int is_rejected_in_playout(Position* pos, Point pt) 
{
    if (point_color(pos, pt) != EMPTY)
        return 1;

    return is_rejected_by_height(pos, pt);
}

int is_rejected_by_height(Position* pos, Point pt) 
{
    int height = line_height(pt, board_size(pos));

    // low move and no stones nearby

    if (height < 2) 
    {
        if (is_empty_area(pos, pt, 5))
            return 1;
    }

    // high move and no stones nearby

    if (height > 5) 
    {
        if (is_empty_area(pos, pt, 5))
            return 1;
    }

    // a move on the 1st line needs either stones
    // of both colours in its remote vicinity
    // or any stone placed much closer

    if (height == 0 && is_empty_area(pos, pt, 3)) 
    {
        Color color = board_color_to_play(pos);

        if (no_stones_of_color(pos, pt, 5, color))
            return 1;

        if (no_stones_of_color(pos, pt, 5, color_other(color)))
            return 1;
    }

    return 0;
}

