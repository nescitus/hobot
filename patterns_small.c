// patterns_small.c -- Routines for 3x3 patterns for hobot program
//
// (c) 2015 Denis Blumstein <db3108@free.fr> Petr Baudis <pasky@ucw.cz>
// MIT licence (i.e. almost public domain)
#include "hobot.h"

// - 3x3 patterns : given the color of the 8 neighbors of the central point 
//   (4 neighbors and 4 diagonal neighbors) the pat3_match() function returns 
//   the answer (yes/no) to the question : 
//   Is there a 3x3 pattern matching at this point ?
//   The recognized patterns are defined by the table pat3_src[] below
//
// ================================ 3x3 patterns ==============================
//
// 1 bit is sufficient to store the fact that a pattern matches for a given 
// configuration of the 8 neighbors color. This configuration can be encoded 
// as a 16 bits integer called env8 (as 2 bits are sufficient to encode one of
// the 4 possible colors of a point). So the set of 3x3 patterns that are 
// recognized is represented by an array of 65536 bits (or 8192 bytes).
//
// The used patterns are symetrical wrt the color so we only need a single array
// (pat3set). A bit is set in this array when at least one pattern matches.
//
// These patterns are used in the random playouts, so pattern matching must be
// very fast. To achieve best speed the 2 components of env8, env4 and env4d,
// are stored as arrays in the struct Position and are incrementally updated by
// the routines that modify the board put_stone() and remove_stone()
//
Byte bit[8]={1,2,4,8,16,32,64,128};
Byte pat3set[8192];     // Set of the pat3 patterns (defined in pat3src below)
int  npat3;             // Number of patterns in pat3src

// A set of 3x3 patterns is defined by an array of ASCII strings (of length 10) 
// with the following encoding
// char pat_src[][10]= {
//       "XOX"                   // X : one of BLACK or WHITE    
//       "..."                   // O : the other color 
//       "???",                  // . : EMPTY
//       "XO."                   // x : not X i.e O or EMPTY or OUT
//       ".X."                   // o : not O i.e X or EMPTY or OUT
//       "?.?",                  // ? : BLACK or WHITE or EMPTY or OUT
//       "###"                   // # : edge of the goban (out of board)
//       "###"
//       "###"                   // string that mark the end of input
// }

char pat3src[][10] = { 
    "XOX"   // 1- hane pattern - enclosing hane
    "..."
    "???",

    "XO."   // 2- hane pattern - non-cutting hane
    "..." 
    "?.?",

    "XO?"   // 3- hane pattern - magari
    "X.." 
    "x.?",

   // "XOO",  // hane pattern - thin hane
   // "...",
   // "?.?", //"X",  - only for the X player

    ".O."   // 4- generic pattern - katatsuke or diagonal attachment; //similar to magari            
    "X.." 
    "...",

    "XO?"   // 5- cut1 pattern (kiri] - unprotected cut
    "O.o" 
    "?o?",
    
    "XO?"   // 6- cut1 pattern (kiri] - peeped cut
    "O.X" 
    "???",
    
    "?X?"   // 7- cut2 pattern (de]
    "O.O" 
    "ooo",
    
    "OX?"   // 8- cut keima
    "o.O" 
    "???",
    
    "X.?"   // 9- side pattern - chase
    "O.?" 
    "##?",
    
    "OX?"   // 10- side pattern - block side cut
    "X.O" 
    "###",
    
    "?X?"   // 11- side pattern - block side connection
    "x.O" 
    "###",
    
    "?XO"   // 12- side pattern - sagari
    "x.x" 
    "###",
    
    "?OX"   // 13- side pattern - cut
    "X.O" 
    "###",
    
    "###" 
    "###" 
    "###"  // Mark the end of the pattern list
};

int nb=0;

int code(char color, int p)
{
    // Bits set for the 4 neighbours North(1), East(5), South(7), West(3)
    // or for the 4 diagonal neighbours NE(2), SE(8), SW(6), NE(0)
    int code_E[4] = {0x00, 0x00, 0x00, 0x00};           // EMPTY(.)
    int code_O[4] = {0x01, 0x02, 0x04, 0x08};           // OUT  (#)
    int code_W[4] = {0x10, 0x20, 0x40, 0x80};           // WHITE(O)
    int code_B[4] = {0x11, 0x22, 0x44, 0x88};           // BLACK(X)
    switch(color) {
        case 'O': return code_W[p];
        case 'X': return code_B[p];
        case '.': return code_E[p];
        case '#': return code_O[p];
    }
    return 0;       // can't happen, but make compiler happy
}

int compute_code(char *src)
// Compute a 16 bits code that completely describes the 3x3 environnement of a
// given point.
// Note: the low 8 bits describe the state of the 4 neighbours, 
//       the high 8 bits describe the state of the 4 diagonal neighbors
{
                                        // src   0 1 2     bits in env8  7 0 4
    int env8=0;                         //       3 4 5     bit 0=LSB     3 . 1
                                        //       6 7 8                   6 2 5
    // Neighbours of the central point
    env8 |= code(src[1], 0);    // North value given by src[1] in position 0
    env8 |= code(src[5], 1);    // East  value given by src[5] in position 1
    env8 |= code(src[7], 2);    // South value given by src[7] in position 2
    env8 |= code(src[3], 3);    // West  value given by src[3] in position 3
    // Diagonal neighbours of the central point
    env8 |= code(src[2], 0)<<8;// North/East value given by src[2] in position 0
    env8 |= code(src[8], 1)<<8;// South/East value given by src[8] in position 1
    env8 |= code(src[6], 2)<<8;// South/West value given by src[6] in position 2
    env8 |= code(src[0], 3)<<8;// North/West value given by src[0] in position 3
    return env8;
}

int pat;
void pat_wildexp(char *src, int i)
// Expand wildchar in src[i]
{
    char src1[10];
    int env8;
    if ( i==9 ) { // all the positions in src are processed -- end of recursion
        env8 = compute_code(src);
        nb++;
        int q = env8 >> 3, r = env8 & 7;
        pat3set[q] |= bit[r];              // set the bit corresponding to env8
        return;
    }
    if (src[i] == '?') {
        strcpy(src1, src);
        src1[i] = 'X'; pat_wildexp(src1, i+1);
        src1[i] = 'O'; pat_wildexp(src1, i+1);
        src1[i] = '.'; pat_wildexp(src1, i+1);
        src1[i] = '#'; pat_wildexp(src1, i+1);
    }
    else if (src[i] == 'x') {
        strcpy(src1, src);
        src1[i]='O'; pat_wildexp(src1, i+1);
        src1[i]='.'; pat_wildexp(src1, i+1);
        src1[i]='#'; pat_wildexp(src1, i+1);
    }
    else if (src[i] == 'o') {
        strcpy(src1, src);
        src1[i]='X'; pat_wildexp(src1, i+1);
        src1[i]='.'; pat_wildexp(src1, i+1);
        src1[i]='#'; pat_wildexp(src1, i+1);
    }
    else 
        pat_wildexp(src, i+1);
}

char *swapcolor(char *src) 
{
    for (int i=0 ; i<9 ; i++) {
        switch (src[i]) {
            case 'X': src[i] = 'O'; break;
            case 'O': src[i] = 'X'; break;
            case 'x': src[i] = 'o'; break;
            case 'o': src[i] = 'x'; break;
        } 
    }
    return src;
}

char* horizflip(char *src)
{
    SWAP(char, src[0], src[6]);
    SWAP(char, src[1], src[7]);
    SWAP(char, src[2], src[8]);
    return src;
}

char* vertflip(char *src)
{
    SWAP(char, src[0], src[2]);
    SWAP(char, src[3], src[5]);
    SWAP(char, src[6], src[8]);
    return src;
}

char* rot90(char *src)
{
    char t=src[0]; src[0]=src[2]; src[2]=src[8]; src[8]=src[6]; src[6]=t;
    t=src[1]; src[1]=src[5]; src[5]=src[7]; src[7]=src[3]; src[3]=t; 
    return src;
}

void pat_enumerate3(char *src)
{
    char src1[10];
    pat_wildexp(src, 0);
    strcpy(src1,src);
    pat_wildexp(swapcolor(src1), 0);
}

void pat_enumerate2(char *src)
{
    char src1[10];
    pat_enumerate3(src);
    strcpy(src1, src);
    pat_enumerate3(horizflip(src1));
}

void pat_enumerate1(char *src)
{
    char src1[10];
    pat_enumerate2(src);
    strcpy(src1, src);
    pat_enumerate2(vertflip(src1));
}

void pat_enumerate(char *src)
{
    char src1[10];
    pat_enumerate1(src);
    strcpy(src1, src);
    pat_enumerate1(rot90(src1));
}

void make_pat3set(void)
// Build the pat3set set (bitfield of 65536 bits). 
// See explanations at top of the file
{
    npat3 = sizeof(pat3src) / 10 - 1;
    if (npat3 > 13) {
        fprintf(stderr,"Error npat3 too big (%d)\n", npat3);
        exit(-1);
    }
    memset(pat3set,0,8192);
    for(int p=0 ; strcmp(pat3src[p], "#########") != 0 ; p++) {
        pat = p;
        pat_enumerate(pat3src[p]);
    }
}
