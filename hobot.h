// hobot -- A minimalistic Go-playing engine (from Petr Baudis michi.py via hobot2)
#include "board.h"

#define EXCLUDE_PATTERNS

//========================= Definition of Data Structures =====================

#define MAX_GAME_LEN (N*N*3)
#define SINGLEPT_OK       1
#define SINGLEPT_NOK      0
#define TWOLIBS_TEST      1
#define TWOLIBS_TEST_NO   0
#define TWOLIBS_EDGE_ONLY 1
#if BOARDSIZE < 141
    #define ZOBRIST_HASH_SIZE 141
#else
    #define ZOBRIST_HASH_SIZE BOARDSIZE
#endif

#define MAX_LIB_CNT 10

extern Color hobot_color;
extern int global_simulation_count;
extern char buf[40000];

// ---------------------------- MCTS Constants --------------------------------
extern int   N_SIMS, MAX_TIME, RAVE_EQUIV, EXPAND_VISITS;
extern int   PRIOR_EVEN, PRIOR_SELFATARI, PRIOR_CAPTURE_ONE, PRIOR_CAPTURE_TWO, PRIOR_CAPTURE_MANY;
extern int   PRIOR_ATARI, PRIOR_DBL_ATARI;
extern int   PRIOR_PAT3, PRIOR_LARGEPATTERN, LEN_PRIOR_CFG, PRIOR_EMPTYAREA, PRIOR_OWNER;
extern int   lib_shortage_bonus[11];
extern int   PRIOR_CFG[], LEN_PRIOR_CFG;
extern int   REPORT_PERIOD;
extern int   max_book_depth;
extern double PROB_HEURISTIC_CAPTURE, PROB_HEURISTIC_PAT3, PROB_HEURISTIC_LARGE_PAT;
extern double PROB_SSAREJECT, PROB_RSAREJECT, PROB_UNIFORM_PLAYOUT;
extern double RESIGN_THRES;
// ----------------------------- Dynamic komi ---------------------------------
extern int DYNKOMI_PERIOD;
extern double DYNKOMI_GREEN;
//------------------------------Unexposed constants----------------------------
extern double URGENT_PATTERN_SCORE;
extern double TENUKI_PATTERN_SCORE;
//------------------------------- Data Structures -----------------------------
typedef struct {
    char    name[2][32];    // players name 
    char    rank[2][4];     // players rank
    char    rules[32];      // rules  as read in the sgf 
    char    result[32];     // result as read in the sgf (ex: W+Time, W+1.5)
    char    overtime[32];   // overtime as read in the sgf
} GameInfo;

typedef struct {
    Position *pos;
    int      handicap;
    float    komi;
    Color    computer_color;
    int      time_left;          // main time left: maintained internally
    int      time_left_gtp;      // main time left: provided   by time_left
    int      time_init;          // main time: initialized     by time_settings
    GameInfo gi;
    Point    moves[MAX_GAME_LEN];
    Point    placed_black_stones[BOARDSIZE];
    Point    placed_white_stones[BOARDSIZE];
    ZobristHash zhash;
    ZobristHash zhistory[MAX_GAME_LEN];
} Game;

typedef struct tree_node { // ------------ Monte-Carlo tree node --------------
    int depth;        // distance from the root
    int visits;       // number of visits
    int wins;         // number of wins(expected reward is w/v)
    int prior_visits; // pv, pw are prior values 
    int prior_wins;   // (node value = w/v + pw/pv)
    int amaf_visits;  // av, aw are amaf values ("all moves as first"),
    int amaf_wins;    // used for the RAVE tree policy)
    int nchildren;    // number of children
    Point move;       // move that reaches the position represented by the node
    struct tree_node **children;
} TreeNode;         //  Monte-Carlo tree node

typedef struct {
    double pessimistic;
    double raw;
    double optimistic;
} Score;

// -------------------------------- Global Data -------------------------------
extern Byte         pat3set[8192];
extern int          saved_nodes;
extern int          npat3;
extern Byte*        pat3set_p[15];
extern Mark*        already_suggested;
extern int          play_until_the_end;        // O or 1
extern int          use_dynamic_komi;
extern double       komi_per_handicap_stone;
extern char         Live_gfx[80];
extern int          Live_gfx_interval;
extern Point        allpoints[BOARDSIZE];
extern float        nplayouts, nplayouts_per_second;
extern float        start_playouts_sec, stop_playouts_sec;
extern int          nplayouts_real;
extern float        saved_time_left;
extern float        best2, bestr, bestwr;
extern ZobristHash  zobrist_hashdata[ZOBRIST_HASH_SIZE][4];

//================================== Code =====================================
//------------------------- Functions in control.c ----------------------------
void   compute_all_status(Position *pos, int owner_map[BOARDSIZE],
        int score_count[], Status block_status[], Status point_status[]);
Score  final_score(Game *game, Status block_status[MAX_BLOCKS], 
                                          Status point_status[BOARDSIZE]);
Point  genmove (Game *game, TreeNode **tree, int *owner_map, int *score_count);
//-------------------------- Functions in debug.c -----------------------------
char*  debug(Game *game, char *command);
//-------------------------- Functions in detect.c ----------------------------
int    line_height(Point pt, int size);
int    is_corner(Point pt, int size);
int    empty_area(Position* pos, Point pt, int dist);
int    no_stones_of_color(Position* pos, Point pt, int dist, Color color);
int    get_min_libs(Position* pos, Point pt);
int    gey_min_libs_for(Position* pos, Point pt, Color c);
//-------------------------- Functions in hobot.c -----------------------------
TreeNode* best_move(TreeNode *tree, TreeNode **except);
void   collect_infos(TreeNode *tree, int n, TreeNode *best
                                       , TreeNode *workspace[], Position *pos);
void   compute_best_moves(TreeNode *tree, char sep[2]
                                        , TreeNode *best_node[5], char *can);
void   compute_principal_variation(TreeNode *tree, Point moves[6]);
int    fix_atari(Position *pos, Point pt, int singlept_ok
        , int twolib_test, int twolib_edgeonly, Slist moves, Slist sizes);
void   free_tree(TreeNode *tree);
int    gen_playout_moves_capture(Position *pos, Slist heuristic_set, float prob,
                                int expensive_ok, Slist moves, Slist sizes);
int    gen_playout_moves_pat3(Position *pos, Slist heuristic_set, float prob,
                                                                  Slist moves);
double mcplayout(Position *pos, int amaf_map[], int owner_map[],
                                        int score_count[2*N*N], int disp);
double hobot_playout(Position* pos, int amaf_map[], int owner_map[],
    int score_count[2 * N * N], int disp);
double random_playout(Position* pos, int amaf_map[], int owner_map[],
    int score_count[2 * N * N], int disp);
TreeNode* new_tree_node(void);
void   print_pos(Position *pos, FILE *f, int *owner_map);
Point  tree_search(Position *pos, TreeNode *tree, int n, int owner_map[],
                                             int score_count[], int disp);
//--------------------------- Functions in komi.c -----------------------------
void adjust_komi(Position* pos, double best_wr);
//-------------------------- Functions in reject.c ----------------------------
int is_rejected_in_tree(Position* pos, Point pt);
int is_rejected_by_height(Position* pos, Point pt);
//-------------------------- Functions in params.c ----------------------------
void   make_params_default(FILE *f);
char*  param_general(const char *param);
char*  param_playout(const char *param);
char*  param_tree(const char *param);
//------------------------- Functions in patterns.c ---------------------------
void   free_large_patterns(void);
void   make_pat3set(void);
char*  make_list_pat3_matching(Position *pos, Point pt);
char*  make_list_pat_matching(Point pt, int verbose);
void   init_large_patterns(const char *prob, const char *spat);
void   log_hashtable_synthesis();
double large_pattern_probability(Point pt, Point ko);
double large_pattern_prob_no_stats(Point pt, Point ko);
void   init_zobrist_hashdata(void);
//---------------------------- Functions in sgf.c -----------------------------
Game*  new_game(Position *pos);
void   free_game(Game *game);
char*  game_clear_board(Game *game);
void   game_set_komi(Game *game, float komi);
int    is_game_board_empty(Game *game);
char*  do_play(Game *game, Color c, Point pt);
char*  do_undo(Game *game);
char*  loadsgf(Game *game, const char *filename, int nmoves);
char*  storesgf(Game *game, const char *filename, const char* version);
//---------------------------- Functions in ui.c ------------------------------
void   display_live_gfx(Position *pos, TreeNode *tree,
                                                     int owner_map[BOARDSIZE]);
void gtp_io(Game *game, FILE *f, FILE *out, int owner_map[], int score_count[]);
//-------------------------- Functions in fuseki.c ----------------------------
Point get_fuseki_move(Position* pos);
Point get_fuseki_guide_move(Position* pos);
Point get_move(Position* pos, char* sequence, char* moves, int nstones);
Point choice_from_str(char* str, int rot);
Point create_pt(int x, int y);
int split(char* str, char c, char*** arr);

//-------------------- Functions inlined for simplicity -----------------------
__INLINE__ int   game_handicap(Game *game) { return game->handicap;}
__INLINE__ float game_komi(Game *game) { return game->pos->komi;}
__INLINE__ float game_main_time(Game *game) { return game->time_init;}
__INLINE__ char* game_name(Game *game, Color c) { return game->gi.name[c&1];}
__INLINE__ char* game_overtime(Game *game) { return game->gi.overtime;}
__INLINE__ char* game_rank(Game *game, Color c) { return game->gi.rank[c&1];}
__INLINE__ char* game_result(Game *game) { return game->gi.result;}
__INLINE__ char* game_rules(Game *game) { 
    if (strlen(game->gi.rules)==3)
        return game->gi.rules;
    else if (strcmp(game->gi.rules,"Japanese")==0)
        return "JPN";
    else if (strcmp(game->gi.rules,"Chinese")==0)
        return "CHN";
    else 
        return "???";
}
//-------------------- Functions inlined for performance ----------------------
__INLINE__ int  is_time_limited(Game *game) {return game->time_init >0;}

// Pattern matching
__INLINE__ int pat3_match(Position *pos, Point pt)
// Return 1 if a 3x3 pattern match at point pt, else 0
{
    int env8=point_env8(pos, pt), q=env8 >> 3, r=env8 & 7;
    return (pat3set[q] & bit[r]) != 0;
}

// function to be assigned to their respective files

Point get_best_prior(Position* pos, TreeNode* tree);
