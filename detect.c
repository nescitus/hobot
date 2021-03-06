// hobot.c -- Poor Go-playing engine

#include "hobot.h"
static int   delta[] = { -N - 1,   1,  N + 1,   -1, -N,  W,  N, -W };

int line_height(Point pt, int size)
// Return the line number above nearest board edge (0 based)
{
    div_t d = div(pt, N + 1);
    int row = d.quot, col = d.rem;
    if (row > size / 2) row = size + 1 - row;
    if (col > size / 2) col = size + 1 - col;
    if (row < col) return row - 1;
    else           return col - 1;
}

int is_corner(Point pt, int size)
{
    div_t d = div(pt, N + 1);
    int row = d.quot, col = d.rem;
    if (row > size / 2) row = size + 1 - row;
    if (col > size / 2) col = size + 1 - col;
    row = row - 1;
    col = col - 1;

    if ((row == 2 || row == 3) && (col == 2 || col == 3)) {
        return 1;
    }
    return 0;
}

int empty_area(Position* pos, Point pt, int dist)
// Check whether there are any stones in Manhattan distance up to dist
{
    int   k;
    Point n;
    FORALL_NEIGHBORS(pos, pt, k, n) {
        if (point_is_color(pos, n))
            return 0;
        else if (point_color(pos, n) == EMPTY
             && dist > 1 && !empty_area(pos, n, dist - 1))
            return 0;
    }
    return 1;
}

int no_stones_of_color(Position* pos, Point pt, int dist, Color color)
// Check whether there are any stones in Manhattan distance up to dist
{
    int   k;
    Point n;
    FORALL_NEIGHBORS(pos, pt, k, n) {

        if (point_is_color(pos, n, color))
            return 0;

        else if (point_color(pos, n) == EMPTY
             || point_color(pos, n) == color) {
            if (dist > 1
                && !no_stones_of_color(pos, n, dist - 1, color))
                return 0;
        }
    }

    return 1;
}


int get_min_libs(Position* pos, Point pt) {

    if (point_color(pos, pt) != EMPTY)
        return 10;

    int k;
    Point new;
    int min_libs = 10;
    FORALL_NEIGHBORS(pos, pt, k, new) {
        if (point_color(pos, new) != EMPTY) {
            
            if (point_color(pos, new) == OUT)
                continue; // fixes edge

            Block b = point_block(pos, new);

            int libs = block_nlibs(pos, b);
            if (libs < min_libs)
                min_libs = libs;
        }
    }

    return min_libs;
}