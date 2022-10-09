// patterns_large.c -- Routines for large patterns for hobot program
//
// (c) 2015 Denis Blumstein <db3108@free.fr> Petr Baudis <pasky@ucw.cz>
// MIT licence (i.e. almost public domain)
#include "hobot.h"

// - Large patterns : given the color of the points in a certain neighborhood 
//   of the central point, the large_pattern_probability() function returns the
//   probability of play at this position. 
//   This probability is used to bias the search in the MCTS tree. 
//
//   The point neighborhoods are defined in [4] (see references in hobot.c). 
//   They are symetric under reflexions and rotations of the board. For each
//   point, 12 neighborhoods of increasing size are considered, each 
//   neighborhood includes all the neighborhoods of lesser size.
//   In the program, the neighborhoods are defined by the 2 tables :
//   pat_gridcular_seq and pat_gridcular_size. These tables are compiled into 
//   the pat_gridcular_seq1d array.

// =============================== Large patterns =============================
//
// The sizes of the neighborhoods are large (up to 141 points). Therefore the
// exact configuration of the colors in the neighborhoods cannot be used for
// pattern matching. Instead, a Zobrist signature (see [4] et [7]) of 64 bits
// is computed from all points in the neighborhoods. Then this signature is 
// searched in a big hash table that contains the signatures of the patterns
// read in the file patterns.spat. If successful, the search returns the 
// probability of the patterns. 
//
// A large board with 7 layers of OUT of board points is used in order to avoid
// tests during the computation of the signature for a given point. This large
// board contains only the information on the color of points. It is build by
// copy_to_large_board() which is called only once in the routine expand() while
// pat_match() is called many times. 
// 
// With the large board it is an easy matter to compute the signature by 
// looping on all the points of the neighborhood thanks to the gridcular_seq1d
// array which stores the displacements with respect to the central point.
//
// The hash table "patterns" is computed by init_patterns(). 
// It uses internal chaining with double hashing [9].
// The performance of this hash table is reported in the log file hobot.log 
// after the compilation of patterns.spat file and at the end of the execution.
//
// ------------------------ Data Structures -----------------------------------
// Large pattern entry in the hash table
typedef struct hash_t {
    ZobristHash   key;      // 64 bits Zobrist hash
    int           id;       // id of the pattern
    float         prob;     // probability of move triggered by the pattern
} LargePat;
#define KSIZE      25         // key size in bits
#define LENGTH     (1<<KSIZE) // Size of the hash table
#define KMASK      (LENGTH-1) // Mask to get the key from the hash signature
#define FOUND      -1

// Displacements with respect to the central point
typedef struct shift_t { int x, y; } Shift;

//---------------------------- Global Data ------------------------------------
Shift pat_gridcular_seq[] = {
    {0,0},      // d=1,2,3 is not considered separately                size 1
    {0,1}, {0,-1}, {1,0}, {-1,0}, {1,1}, {-1,1}, {1,-1}, {-1,-1},
    {0,2}, {0,-2}, {2,0}, {-2,0},                                   // size 2
    {1,2}, {-1,2}, {1,-2}, {-1,-2}, {2,1}, {-2,1}, {2,-1}, {-2,-1}, // size 3
    {0,3}, {0,-3}, {2,2}, {-2,2}, {2,-2}, {-2,-2}, {3,0}, {-3,0},   // size 4
    {1,3}, {-1,3}, {1,-3}, {-1,-3}, {3,1}, {-3,1}, {3,-1}, {-3,-1}, // size 5
    {0,4}, {0,-4}, {2,3}, {-2,3}, {2,-3}, {-2,-3}, {3,2}, {-3,2},   // size 6
    {3,-2}, {-3,-2}, {4,0}, {-4,0},
    {1,4}, {-1,4}, {1,-4}, {-1,-4}, {3,3}, {-3,3}, {3,-3}, {-3,-3}, // size 7
    {4,1}, {-4,1}, {4,-1}, {-4,-1},
    {0,5}, {0,-5}, {2,4}, {-2,4}, {2,-4}, {-2,-4}, {4,2}, {-4,2},   // size 8
    {4,-2}, {-4,-2}, {5,0}, {-5,0},
    {1,5}, {-1,5}, {1,-5}, {-1,-5}, {3,4}, {-3,4}, {3,-4}, {-3,-4}, // size 9
    {4,3}, {-4,3}, {4,-3}, {-4,-3}, {5,1}, {-5,1}, {5,-1}, {-5,-1},
    {0,6}, {0,-6}, {2,5}, {-2,5}, {2,-5}, {-2,-5}, {4,4}, {-4,4},   // size 10
    {4,-4}, {-4,-4}, {5,2}, {-5,2}, {5,-2}, {-5,-2}, {6,0}, {-6,0},
    {1,6}, {-1,6}, {1,-6}, {-1,-6}, {3,5}, {-3,5}, {3,-5}, {-3,-5}, // size 11
    {5,3}, {-5,3}, {5,-3}, {-5,-3}, {6,1}, {-6,1}, {6,-1}, {-6,-1},
    {0,7}, {0,-7}, {2,6}, {-2,6}, {2,-6}, {-2,-6}, {4,5}, {-4,5},   // size 12
    {4,-5}, {-4,-5}, {5,4}, {-5,4}, {5,-4}, {-5,-4}, {6,2}, {-6,2},
    {6,-2}, {-6,-2}, {7,0}, {-7,0}
};
int pat_gridcular_seq1d[141];
int pat_gridcular_size[13] = {0,9,13,21,29,37,49,61,73,89,105,121,141};
int large_patterns_loaded = 0;
// Primes used for double hashing in find_pat()
int primes[32]={5,      11,    37,   103,   293,   991, 2903,  9931,
                7,      19,    73, 10009, 11149, 12553, 6229, 10181,
                1013, 1583,  2503,  3491,  4637,  5501, 6571,  7459,
                8513, 9433, 10433, 11447, 11887, 12409, 2221,  4073};

int         color[256];
ZobristHash zobrist_hashdata[ZOBRIST_HASH_SIZE][4];
LargePat*   patterns;
float*      probs;
long long   nsearchs=0;
long long   nsuccess=0;
double      sum_len_success=0.0;
double      sum_len_failure=0.0;

// Code: ------ Dictionnary of patterns (hastable with internal chaining) -----
void print_pattern(const char *msg, int i, LargePat p)
{
    sprintf(buf,"%s%-6d %16.16llx %6d %f", msg, i, p.key, p.id, p.prob);
}

void dump_patterns()
{
    printf("Large patterns hash table\n");
    for (int i=0 ; i<LENGTH ; i++) {
        print_pattern("", i, patterns[i]);
        printf("%s\n", buf);
    }
}

int find_pat(ZobristHash key)
// Return the slot where key is found or the empty slot where key should go 
{
    assert(key!=0);

    int h = (key>>20) & KMASK, h2=primes[(key>>(20+KSIZE)) & 15], len=1;
    nsearchs++;
    while (patterns[h].key != key) {
        if (patterns[h].key == 0) {
            sum_len_failure += len;
            return h;
        }
        len++;
        h+=h2; if (h>LENGTH) h -= LENGTH;
    }
    nsuccess++;
    sum_len_success += len;
    return h;
}

int insert_pat(LargePat p)
// Insert a pattern in the hash table. Return FOUND if the pattern is already in
{
    int i = find_pat(p.key);
    if (patterns[i].key==0) {
        patterns[i] = p;
        return i;
    }
    else
        return FOUND;
}

LargePat build_pat(ZobristHash key, int id, float prob)
{    
    LargePat pat = {key, id, prob};
    return pat;
}

// Code: ------------------- Zobrist signature computation --------------------
void init_stone_color(void)
{
    memset(color,0,1024);
    color['.'] = 0;                      // 0: EMPTY
    color['#'] = color[' '] = 1;         // 1: OUT  
    color['O'] = color['x'] = 2;         // 2: Other or 'x'
    color['X'] = 3;                      // 3: ours 
}

void init_zobrist_hashdata(void)
{
    int idum_save = idum;
    idum = 55555; // make sure zobrist_hashdata does not depend on user input
                  // 55555 seems to work reasonably well
    for (int d=0 ; d<ZOBRIST_HASH_SIZE ; d++)  {//d = displacement ...
        for (int c=0 ; c<4 ; c++) {
            unsigned int d1 = qdrandom(), d2=qdrandom();
            ZobristHash ld1 = d1, ld2 = d2;
            zobrist_hashdata[d][c] = (ld1<<32) + ld2;
        }
    }
    idum = idum_save;
}
  
ZobristHash zobrist_hash(char *pat)
// Return the Zobrist signature of a large pattern provided as ASCII string
{
    int l = (int)strlen(pat);
    ZobristHash k=0;
    for (int i=0 ; i<l ; i++) {
        k ^= zobrist_hashdata[i][color[pat[i]]];
    }
    return k;
}

ZobristHash 
update_zobrist_hash_at_point(Point pt, int size, ZobristHash k)
// Update the Zobrist signature for the points of pattern of size 'size' 
{
    int imin=pat_gridcular_size[size-1], imax=pat_gridcular_size[size];
    for (int i=imin ; i<imax ; i++) {
        int c = color[large_board[pt+pat_gridcular_seq1d[i]]];
        k ^= zobrist_hashdata[i][c];
    }
    return k;
}

// Code: -------------- rotation and reflexion of the patterns ----------------
void init_gridcular(Shift seq[141], int seq1d[141]) {
    for (int i=0 ; i<141 ; i++)
        seq1d[i] = seq[i].x - seq[i].y*(N+7);
}

int nperms=0;       // current permutation

int permutation_OK(int p[8][141])
{
    for (int i=0 ; i<141 ; i++)
        if (p[0][i] != i) return 0;
    return 1;
}

int gridcular_index(int disp)
{
    for (int i=0 ; i<141 ; i++)
        if (pat_gridcular_seq1d[i] == disp)
            return i;
    log_fmt_s('E', "gridcular_index(): can't happen",NULL);
    return -1;
}

void gridcular_register(Shift seq[141], int p[8][141])
{
    int seq1d[141];
    init_gridcular(seq,seq1d);
    for (int i=0 ; i< 141 ; i++)
        p[nperms][i] = gridcular_index(seq1d[i]);
    nperms++;
}

void gridcular_enumerate2(Shift seq[141], int p[8][141])
{
    Shift seq1[141];
    gridcular_register(seq, p);
    // Horizontal flip of the pattern
    for (int i=0 ; i<141 ; i++) {
        seq1[i].x =  seq[i].x;
        seq1[i].y = -seq[i].y;
    }
    gridcular_register(seq1, p);
}

void gridcular_enumerate1(Shift seq[141], int p[8][141])
{
    Shift seq1[141];
    gridcular_enumerate2(seq, p);
    // Vertical flip of the pattern
    for (int i=0 ; i<141 ; i++) {
        seq1[i].x = -seq[i].x;
        seq1[i].y =  seq[i].y;
    }
    gridcular_enumerate2(seq1, p);
}

void gridcular_enumerate(int p[8][141])
{
    Shift seq1[141];
    gridcular_enumerate1(pat_gridcular_seq, p);
    // 90 deg rotation of the pattern
    for (int i=0 ; i<141 ; i++) {
        seq1[i].x = -pat_gridcular_seq[i].y;
        seq1[i].y =  pat_gridcular_seq[i].x;
    }
    gridcular_enumerate1(seq1, p);
}

void permute(int permutation[8][141],int i,char strpat[256],char strperm[256])
{
    int len = (int)strlen(strpat);
    for (int k=0 ; k<len ; k++)
        strperm[k] = strpat[permutation[i][k]];
    strperm[len] = 0;
}

// Code: -------------------- load pattern definitions ------------------------
int max_pattern_id(FILE *f)
// Determine the size of the large pattern database (max of id)
{
    float prob;
    int   id, id_max=0,t1,t2;

    while (fgets(buf, 255, f) != NULL) {
        if (buf[0] == '#') continue;
        sscanf(buf,"%f %d %d (s:%d)", &prob, &t1, &t2, &id);
        if (id>id_max)
            id_max = id;
    }
    rewind(f);
    return id_max;
}

void load_prob_file(FILE *f)
// Load the probabilities of large patterns
{
    float prob;
    int   id,t1,t2;
    int zeroed = 0;
    int always_zero = 0;
    int urgent = 0;
    int overwrite_urgent = 0;

    while (fgets(buf, 255, f) != NULL) {
        
        // skip comment lines
        
        if (buf[0] == '#') 
            continue;

        sscanf(buf,"%f %d %d (s:%d)", &prob, &t1, &t2, &id);
        if (probs[id] >= URGENT_PATTERN_SCORE 
        && prob < URGENT_PATTERN_SCORE) {
            overwrite_urgent++;
        }
        if (probs[id] > 0 && prob == 0) 
            zeroed++;
        if (probs[id] == 0 && prob == 0)
            always_zero++;

        probs[id] = prob;

        if (probs[id] >= URGENT_PATTERN_SCORE)
            urgent++;
    }

    sprintf(buf, "Zeroed %d potentially useful patterns, %d are always 0", 
            zeroed, always_zero);
    log_fmt_s('I', buf, NULL);
    sprintf(buf, "%d patterns seem urgent, %d of them get overwritten",
        urgent, overwrite_urgent);
    log_fmt_s('I', buf, NULL);
}

int load_spat_file(FILE *f)
// Load the spatial description of large patterns
{
    int  d, id, idmax=-1, len, lenmax=0, npats=0;
    char strpat[256], strperm[256];
    ZobristHash k;
    int permutation[8][141];

    // compute the 8 permutations of the gridcular positions corresponding
    // to the 8 possible reflexions or rotations of pattern
    gridcular_enumerate(permutation);

    while (fgets(buf, 255, f) != NULL) {
        if (buf[0] == '#') continue;
        sscanf(buf,"%d %d %s", &id, &d, strpat);
        npats++;
        len = (int)strlen(strpat);
        if (len > lenmax) {
            lenmax = len;
            idmax = id;
        }
        if (id > idmax)
            idmax = id;
        for (int i=0 ; i< 8 ; i++) {
            permute(permutation, i, strpat, strperm);
            assert(permutation_OK(permutation));
            k = zobrist_hash(strperm);
            LargePat pat = build_pat(k, id, probs[id]);
            insert_pat(pat);
        }
    }
    log_fmt_i('I', "read %d patterns", npats);
    log_fmt_i('I', "idmax = %d", idmax);
    sprintf(buf, "pattern length max = %d (found at %d)", lenmax, idmax);
    log_fmt_s('I', buf, NULL);
    large_patterns_loaded = 1;
    return npats;
}

// Code: ------------------------- Public functions ---------------------------
void log_hashtable_synthesis() 
{
    double nkeys=0;
    if (!large_patterns_loaded) return;
    for (int i=0 ; i<LENGTH ; i++) 
        if(patterns[i].key != 0) nkeys +=1.0;
    sprintf(buf,"hashtable entries: %.0lf (fill ratio: %.1lf %%)", nkeys,
                                             100.0 * nkeys / LENGTH);
    log_fmt_s('I', buf, NULL);
    sprintf(buf,"%lld searches, %lld success (%.1lf %%)", nsearchs, nsuccess,
                                        100.0 * (double) nsuccess / nsearchs);
    log_fmt_s('I', buf, NULL);
    sprintf(buf,"average length of searchs -- success: %.1lf, failure: %.1lf",
            sum_len_success/nsuccess, sum_len_failure/(nsearchs-nsuccess));
    log_fmt_s('I', buf, NULL);
}

void init_large_patterns(const char *prob, const char *spat)
// Initialize all the data necessary to use large patterns 
{
    FILE *fspat=NULL, *fprob=NULL;    // Files containing large patterns

    // Initializations
    init_zobrist_hashdata();
    init_stone_color();
    init_gridcular(pat_gridcular_seq, pat_gridcular_seq1d);
    init_large_board();

    // Load patterns data from files
    patterns = hobot_calloc(LENGTH, sizeof(LargePat));
    log_fmt_s('I', "Loading pattern probs ...", NULL);
    fprob = fopen(prob, "r");
    if (fprob == NULL)
        log_fmt_s('w', "Cannot load pattern file:%s","patterns.prob");
    else {
        int id_max = max_pattern_id(fprob);
        log_fmt_i('I', "Reading patterns (id_max = %d)", id_max);
        probs = hobot_calloc(id_max+1, sizeof(float));
        load_prob_file(fprob);
        fclose(fprob);

        log_fmt_s('I', "Loading pattern spatial dictionary ...", NULL);
        fspat = fopen(spat, "r");
        if (fspat == NULL)
            log_fmt_s('w', "Warning: Cannot load pattern file:%s","patterns.spat");
        else {
            load_spat_file(fspat);
            fclose(fspat);
        }
    }
    if (verbosity > 0 && (fprob == NULL || fspat == NULL)) {
        fprintf(stderr, "Warning: hobot cannot load pattern files, "
                "It will be much weaker. ");
        if (EXPAND_VISITS > 2)
            fprintf(stderr, "Consider lowering EXPAND_VISITS %d->2\n",
                    EXPAND_VISITS);
        else
            fprintf(stderr,"\n");
    }
    log_fmt_s('I', "=========== Hashtable initialization synthesis ==========",
                                                                         NULL);
    // reset the statistics after logging them 
    log_hashtable_synthesis();
    nsearchs = nsuccess = 0;
    sum_len_success=sum_len_failure=0.0;
}

void free_large_patterns(void)
{
    free(probs);
    free(patterns);
}

double large_pattern_probability(Point pt)
// return probability of large-scale pattern at coordinate pt. 
// Multiple progressively wider patterns may match a single coordinate,
// we consider the largest one.
{
    double prob=-1.0;
    int matched_len=0, non_matched_len=0;
    ZobristHash k=0;
    double last_prob = 0;
    int last_key = 0;
    int prev_key = 0;
    int size = 0;

    if (large_patterns_loaded)
        for (int s=1 ; s<13 ; s++) {
            int len = pat_gridcular_size[s];
            k = update_zobrist_hash_at_point(large_coord[pt], s, k);
            int i = find_pat(k);
            if (patterns[i].key==k) {
                size = s;
                last_prob = prob;
                prob = patterns[i].prob;
                prev_key = last_key;
                last_key = patterns[i].id;

                if (patterns[i].id >= 1064482 && patterns[i].id < 1066922) {
                    sprintf(buf, "Added pattern no. %d is used", last_key);
                    log_fmt_s('U', buf, NULL);
                }

                matched_len = len;
            }
            else if (matched_len < non_matched_len && non_matched_len < len)
                break;
            else
                non_matched_len = len;
        }

    // Safeguard against zeroing patterns 
    // that seem important based on high score
    // on the previous size; also log such situations.

    if (last_prob >= 0.200 && last_prob < 10.0 && prob == 0) {
        prob = last_prob;
        if (size >= 10) {
            sprintf(buf, "Predecessor: %d scored %f, adjusted pattern: %d", prev_key, last_prob, last_key);
            log_fmt_s('X', buf, NULL);
        }
    }

    return prob;
}

double large_pattern_prob_no_stats(Point pt)
// return probability of large-scale pattern at coordinate pt. 
// Multiple progressively wider patterns may match a single coordinate,
// we consider the largest one.
{
    double prob = -1.0;
    int matched_len = 0, non_matched_len = 0;
    ZobristHash k = 0;
    double last_prob = 0;
    int size = 0;

    if (large_patterns_loaded)
        for (int s = 1; s < 13; s++) {
            int len = pat_gridcular_size[s];
            k = update_zobrist_hash_at_point(large_coord[pt], s, k);
            int i = find_pat(k);
            if (patterns[i].key == k) {
                size = s;
                last_prob = prob;
                prob = patterns[i].prob;
                matched_len = len;
            }
            else if (matched_len < non_matched_len && non_matched_len < len)
                break;
            else
                non_matched_len = len;
        }

    // Safeguard against zeroing patterns 
    // that seem important based on high score
    // on the previous size; also log such situations.

    if (last_prob >= 0.200 && last_prob < 10.0 && prob == 0) {
        prob = last_prob;
    }

    return prob;
}

char* make_list_pat_matching(Point pt, int verbose)
// Build the list of large patterns that match at the point pt
{
    ZobristHash k=0;
    int i;
    char id[16];

    if (!large_patterns_loaded) return "";

    buf[0] = 0;
    for (int s=1 ; s<13 ; s++) {
        k = update_zobrist_hash_at_point(large_coord[pt], s, k);
        i = find_pat(k); 
        if (patterns[i].key == k) {
            sprintf(id,"%d(%.3f) ", patterns[i].id, patterns[i].prob);
            strcat(buf, id);
        }
    }
    return buf;
}
