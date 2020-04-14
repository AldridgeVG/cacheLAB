#include "cachelab.h"
#include "errno.h"
#include "getopt.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"

//[set1(line1,lie2), set2(line...)]
typedef struct {
    int valid;
    uint64_t tag;
    uint64_t accessCount;
} line;

typedef line *entryLine;

typedef entryLine *entrySet;

// use a struct to save results
typedef struct {
    int hit;
    int miss;
    int evic;
} result;

// init a cache(muti sets)
entrySet initializeCache(uint64_t S, uint64_t E) {
    entrySet cache;

    // here use calloc but not malloc to tackle the default(S=0,E=0)
    // init set
    if ((cache = calloc(S, sizeof(entryLine))) == NULL) {
        perror("Failed to calloc entry_of_sets");
        exit(0);
    }

    // init line in each set
    for (int i = 0; i < S; ++i) {
        if ((cache[i] = calloc(E, sizeof(line))) == NULL) {
            perror("Failed to calloc line in sets");
        }
    }
    return cache;
}

// release mem of a cache
void realseMem(entrySet cache, uint64_t S, uint64_t E) {
    for (int i = 0; i < S; ++i) {
        free(cache[i]);
    }
    free(cache);
}

// hit/miss/eviction count
result HME(entryLine tgtLine, result Res, uint64_t E, uint64_t tag,
           int verbose) {
    uint64_t oldest = UINT64_MAX;
    uint64_t newest = 0;
    uint64_t oldestNum = UINT64_MAX;

    uint64_t hit = 0;

    // i match the type of E: 64bit unsigned
    uint64_t i;
    for (i = 0; i < E; ++i) {
        // hit
        if (tgtLine[i].tag == tag && tgtLine[i].valid) {
            if (verbose) printf("hit!\n");
            hit = 1;
            Res.hit++;
            tgtLine[i].accessCount++;
            break;
        }
    }

    // miss
    if (hit == 0) {
        if (verbose) printf("miss!\n");
        Res.miss++;

        // search for the oldest block(not recently used)
        for (i = 0; i < E; ++i) {
            if (tgtLine[i].accessCount < oldest) {
                // set current access as oldest and record
                oldest = tgtLine[i].accessCount;
                oldestNum = i;
            }
            if (tgtLine[i].accessCount > newest) {
                // update newest if there's bigger number of cnt
                newest = tgtLine[i].accessCount;
            }
        }

        // replace the oldest block and cnt as newest++ to signify just accessd
        tgtLine[oldestNum].accessCount = newest + 1;
        tgtLine[oldestNum].tag = tag;

        // if oldest is valid then ther was an eviction
        // if it was valid ,now still valid , no change
        if (tgtLine[oldestNum].valid) {
            if (verbose) printf(" and eviction!\n");
            Res.evic++;
        }

        // if invalid then replace as normal
        // change valid to 1
        else {
            if (verbose) printf("\n");
            // update this block as valid
            tgtLine[oldestNum].valid = 1;
        }
    }
    return Res;
}

// CORE FUNC-- test his/mis/evc
result readTest(FILE *tracefile, entrySet cache, uint64_t s, uint64_t S,
                uint64_t E, uint64_t b, uint64_t verbose) {
    result Res = {0, 0, 0};

    char op;
    uint64_t address;

    // special fscanf format to get input info
    //%*[^\n] means discard read chars before \n and after %lx
    while ((fscanf(tracefile, " %c %lx%*[^\n]", &op, &address)) == 2) {
        // ignore Instruction load
        if (op == 'I')
            continue;
        else {
            // 2^n-1 -> 000...01111 as mask(n = 4)
            uint64_t indexSet_mask = (1 << s) - 1;
            // get the addr in cache the &mask to get set addr/index
            uint64_t indexSet = (address >> b) & indexSet_mask;
            // get tag
            uint64_t tag = (address >> b) >> s;

            // get set number to look for target line in that set
            entryLine tgtLine = cache[indexSet];

            // L or S has only one access to cache
            if (op == 'L' || op == 'S') {
                if (verbose) printf("%c %lx ", op, address);
                Res = HME(tgtLine, Res, E, tag, verbose);
            }

            // consider M as L then S
            else if (op == 'M') {
                if (verbose) printf("%c %lx ", op, address);

                // L, can be h/m/e
                Res = HME(tgtLine, Res, E, tag, verbose);

                // S, must hit
                Res = HME(tgtLine, Res, E, tag, verbose);
            }

            else
                continue;
        }
    }
    return Res;
}

// main
int main(int argc, char *const argv[]) {
    // init result
    result Result = {0, 0, 0};

    const char *help_message =
        "Wrong Input!\n Usage: \"Your complied program\" [-hv] -s <s> -E <E> "
        "-b <b> -t <tracefile>\n"
        "<s> <E> <b> should all above zero and below 64.\n"
        "Complied with std=c99\n";
    const char *command_options = "hvs:E:b:t:";

    // init trace and cache
    FILE *tracefile = NULL;
    entrySet cache = NULL;

    int verbose = 0;  // 1 to switch to verbose mode, default 0
    uint64_t s = 0;   // number of sets index's bits
    uint64_t S = 0;   // number of sets
    uint64_t E = 0;   // number of lines
    uint64_t b = 0;   // number of blocks index's bits

    // get args
    char tmp;
    while ((tmp = getopt(argc, argv, command_options)) != -1) {
        switch (tmp) {
            // h for help_msg and exit
            case 'h': {
                printf("%s", help_message);
                exit(1);
            }

            // v for verbose mode
            case 'v': {
                verbose = 1;
                break;
            }

            // analyse effective argument
            // s for bits of set_num
            case 's': {
                // assuming there's at least 2 sets(bit 1)
                if (atol(optarg) <= 0) {
                    printf("%s", help_message);
                    exit(0);
                }
                // s is bits and S = 2^s = 1<<s
                s = atol(optarg);
                S = 1 << s;
            }

            // E for num of lins in a set
            case 'E': {
                if (atol(optarg) <= 0) {
                    printf("%s", help_message);
                    exit(0);
                }
                E = atol(optarg);
                break;
            }

            // b for number of bytes in a DRAM section
            case 'b': {
                if (atol(optarg) <= 0) {
                    printf("%s", help_message);
                    exit(0);
                }
                b = atol(optarg);
                break;
            }

            // t's nxt argv signify trace filename
            case 't': {
                if ((tracefile = fopen(optarg, "r")) == NULL) {
                    printf("Failed to open tracefile");
                    exit(0);
                }
                break;
            }

            default: {
                printf("%s", help_message);
                exit(0);
            }
        }
    }

    // invalid input
    if (s == 0 || b == 0 || E == 0 || tracefile == NULL) {
        printf("%s", help_message);
        exit(0);
    }

    // crate cache, test and get result uint64_to result struct
    cache = initializeCache(S, E);

    Result = readTest(tracefile, cache, s, S, E, b, verbose);

    realseMem(cache, S, E);

    printSummary(Result.hit, Result.miss, Result.evic);

    return 0;
}