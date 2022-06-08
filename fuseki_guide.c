// fuseki.c -- Playing the first few opening moves
#include "hobot.h"

Point get_fuseki_guide_move(Position* pos) {

    Point pt = PASS_MOVE;
    int nstones = board_nstones(pos);

    // we're using rather short opening lines

    if (nstones > max_book_depth)
        return PASS_MOVE;

    // initial move: either hoshi or komoku in a "polite" corner

    if (nstones == 0) {
        return create_pt(16 + (random_int(10000) <= 5000), 16);
    }

    // hoshi vs hoshi: double hoshi, Chinese and orthodox fuseki invitation
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 ", "Q4 Q3 R4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D16 ", "Q4 Q3 R4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 C17 ", "Q4 Q3 R4 ", nstones);

    // komoku vs (diagonal) hoshi
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 ",               "D4 D17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 ",            "Q3 Q3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q4 ",         "D16 D17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 ",         "D16 D17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 ",     "P17 F17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 ", "Q5 R5 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 ", "Q5 R5 ", nstones);

    // old style komoku fuseki - keima enclosure
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 Q5 ",  "R5 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 Q5 R5 ", "R6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 Q5 R5 R6 ", "R4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 Q5 R5 R6 R4 ", "Q6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 Q5 R5 R6 R4 Q6 ", "O3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 Q5 R5 R6 R4 Q6 O3 ", "Q10 ", nstones);

    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 Q5 ", "R5 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 Q5 R5 ", "R6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 Q5 R5 R6 ", "R4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 Q5 R5 R6 R4 ", "Q6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 Q5 R5 R6 R4 Q6 ", "O3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 Q5 R5 R6 R4 Q6 O3 ", "Q10 ", nstones);

    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 R5 ", "O4 R9 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 R5 O4 ", "R8 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 R5 O4 R8 ", "F3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 R5 R9 ", "P4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 R5 R9 P4 ", "P3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 R5 R9 P4 P3 ", "O4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 R5 R9 P4 P3 O4 ", "N3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 R5 R9 P4 P3 O4 N3 ", "R3 ", nstones);

    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 R5 ", "O4 R9 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 R5 O4 ", "R8 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 R5 O4 R8 ", "F3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 R5 R9 ", "P4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 R5 R9 P4 ", "P3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 R5 R9 P4 P3 ", "O4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 R5 R9 P4 P3 O4 ", "N3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 R5 R9 P4 P3 O4 N3 ", "R3 ", nstones);

    // counter bad split
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 P17 R10 ", "R6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 P17 R10 ", "R6 ", nstones);

    // old style komoku fuseki - ogeima enclosure
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 ", "Q5 R5 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 Q5 ", "R5 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 Q5 R5 ", "R6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 Q5 R5 R6 ", "R4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 Q5 R5 R6 R4 ", "Q6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 Q5 R5 R6 R4 Q6 ", "O3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 Q5 R5 R6 R4 Q6 O3 ", "Q10 ", nstones);

    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 Q5 ", "R5 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 Q5 R5 ", "R6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 Q5 R5 R6 ", "R4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 Q5 R5 R6 R4 ", "Q6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 Q5 R5 R6 R4 Q6 ", "O3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 Q5 R5 R6 R4 Q6 O3 ", "Q10 ", nstones);

    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 R5 ", "O4 R9 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 R5 O4 ", "R8 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 R5 O4 R8 ", "F3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 R5 R9 ", "P4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 R5 R9 P4 ", "P3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 R5 R9 P4 P3 ", "O4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 R5 R9 P4 P3 O4 ", "N3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 R5 R9 P4 P3 O4 N3 ", "R3 ", nstones);

    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 R5 ", "O4 R9 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 R5 O4 ", "R8 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 R5 O4 R8 ", "F3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 R5 R9 ", "P4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 R5 R9 P4 ", "P3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 R5 R9 P4 P3 ", "O4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 R5 R9 P4 P3 O4 ", "N3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 R5 R9 P4 P3 O4 N3 ", "R3 ", nstones);

    // counter bad split
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D16 O17 R10 ", "R6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q3 D17 O17 R10 ", "R6 ", nstones);

    // komoku + mokuhazushi

    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 R5 ", "D16 D17", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D16 R5 ", "D4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 R5 ", "D4 ", nstones);

    // komoku vs komoku
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 ",           "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 Q3 ",        "D4 P17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 Q3 D4 ",     "D15 P17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 Q3 C3 ",     "D15 P17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 Q3 D3 ",     "D15 P17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 D4 ",        "Q4 Q3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 C4 ",        "Q4 Q3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 Q3 ",            "D16 D17 ", nstones);

    // Shusaku fuseki
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 Q3 P17 ", "C4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 Q3 P17 C4 ", "R5 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 Q3 P17 C4 R5 ", "Q15 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 Q3 P17 C4 R5 Q15 ", "E3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D17 Q3 P17 C4 R5 Q15 E3 ", "R9 ", nstones);
    
    // komoku - other responses
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D3 ",            "D16 D16 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 Q4 ",            "D16 D17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D16 ",           "Q3 D4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D16 Q3 ",        "P17 F17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 D16 ",        "Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q4 ",         "D16 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D4 Q4 D16 O17 ", "R6 ", nstones);

    // hoshi
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 ",               "D4 D16 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 Q3 ",         "D16 D17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 Q3 ",         "D16 D17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 Q4 ",         "D16 D17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 Q4 D16 ",     "Q10 F17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 Q4 D16 F17 ", "C14 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 Q4 D16 F17 C14 ", "K16 ", nstones);

    // sanrensei
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 Q4 D16 Q10 ", "O3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 Q4 D16 Q10 O3 ", "M3 Q6 ", nstones);
 
    // cross hoshi
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D17 ", "D4 Q3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D17 D4 ", "Q3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D17 D4 Q3 ", "Q5 R5 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D17 D4 Q3 Q5 ", "R5 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D17 D4 Q3 Q5 R5 ", "R6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D17 D4 Q3 Q5 R5 R6 ", "R4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D17 D4 Q3 Q5 R5 R6 R4 ", "Q6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D17 D4 Q3 Q5 R5 R6 R4 Q6 ", "O3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D17 D4 Q3 Q5 R5 R6 R4 Q6 O3 ", "Q10 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D17 D4 Q3 Q5 R5 R6 R4 Q6 O3 Q10 ", "O17 ", nstones);

    // playing against misaligned komoku
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 C16 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D3 ",            "C16 D16 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D3 D16 R4 ",     "D5 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D3 D16 Q4 ",     "D5 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 Q3 ",            "D16 D17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 Q4 ",            "D16 D17 ", nstones);

    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 C4 ",         "D16 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 C4 D16 E3 ",  "C9 C10 ", nstones);
    
    // Orthodox fuseki possible
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 R4 ",          "D16 D17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 R4 D16 ",      "O3 F3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 R4 D17 ",      "O3 F3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 R4 D16 P3 ",   "R14 R10 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D4 R4 D17 P3 ",   "R14 R10 ", nstones);

    // Chinese fuseki possible
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D16 Q3 D4 ",     "F3 R9 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D17 Q3 D4 ",     "F3 R9 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D16 Q3 D4 R9 ",  "O17 G3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D17 Q3 D4 R9 ",  "O17 G3 ", nstones);

    // initial san-san
    if (pt == PASS_MOVE) pt = get_move(pos, "R17 ",               "D4 D17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R17 D4 Q3 ",         "D16 D17 ", nstones);

    if (pt == PASS_MOVE) pt = get_move(pos, "K10 ", "Q16 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q5 ", "D16 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q5 D16 P16 ", "D4 ", nstones);

    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 C4 D16 C14 F17 D10 ", "F3 F4 O3 J3 K3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 C4 D16 C14 F17 ", "D10 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 C4 D16 C14 ", "F17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 C4 D16 ", "C14 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 C4 ", "D16 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 ", "C4 ", nstones);

    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 D4 C17 R14 O17 ", "Q10 S16 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 D4 C17 R14 ", "O17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 D4 C17 ", "R14 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 D4 ", "C17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 Q16 ", "D4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q4 ", "Q16 ", nstones);

    if (pt == PASS_MOVE) pt = get_move(pos, "R4 D4 Q17 D16 F3 C6 P4 ", "Q15 R14 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R4 D4 Q17 D16 F3 C6 P3 ", "Q15 R14 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R4 D4 Q17 D16 F3 C6 ", "P3 P4", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R4 D4 Q17 D16 F3 ", "C6 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R4 D4 Q17 D16 ", "F3 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R4 D4 Q17 ", "D16 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R4 D4 ", "Q17 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R4 ", "D4 ", nstones);

    if (pt == PASS_MOVE) pt = get_move(pos, "R5 ", "D4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R5 D4 P17 ", "D16 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R5 D4 P17 D16 F3", "J4 C6 ", nstones);

    // vs unusual 2nd move replies
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 E17 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 E16 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 E17 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 E16 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 C15 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D15 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 C15 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D15 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 C5 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 D5 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 C5 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 D5 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 E3 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "R16 E4 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 E3 ", "Q3 Q4 ", nstones);
    if (pt == PASS_MOVE) pt = get_move(pos, "Q16 E4 ", "Q3 Q4 ", nstones);
    
    return pt;
}