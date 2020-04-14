#include "cachelab.h"
#include "errno.h"
#include "getopt.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

//[set1(line1,lie2), set2(line...)]
typedef struct {
    int valid;
    int tag;
    int accessCount
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
entrySet initializeCache(int S, int E) {
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
void realseMem(entrySet cache, int S, int E) {
    for (int i = 0; i < S; ++i) {
        free(cache[i]);
    }
    free(cache);
}

// hit/miss/eviction count
result HME(entryLine tgtLine, result Res, int E, int tag, int verbose) {
    int oldest = INT64_MAX;
    int newest = 0;
    int oldestNum = INT64_MAX;

    int hit = 0;

    for (int i = 0; i < E; i++) {
        
        //hit
        if(tgtLine[i].tag == tag && tgtLine[i].valid){
            if(verbose) printf("hit!\n");
            hit = 1;
            Res.hit++;
            tgtLine[i].accessCount++;
            break;
        }
    }
        //miss
        if(hit == 0){
            if(verbose) printf("miss!\n");
            Res.miss++;

            //search for the oldest block
            int i;
            for(int i = 1;i<E;i++){
                if(tgtLine[i].accessCount < oldest){
                    
                    //set current access as oldest and record
                    oldest = tgtLine[i].accessCount;
                    oldestNum =i;
                }
                if(tgtLine[i].accessCount>newest){
                    newest = tgtLine[i].accessCount;
                }
            }

            tgtLine[oldestNum].accessCount = newest+1;
            tgtLine[oldestNum].tag = tag;

            //if oldest is valid the eviction
            if(tgtLine[oldestNum].valid){
                if(verbose) printf("eviction!\n");
                Res.evic++;
            }

            //if invalid then replace as normal
            else{
                if(verbose) printf("\n");
                //update this block as valid
                tgtLine[oldestNum].valid = 1;
            }
        }
    return Res;
}

// CORE FUNC-- test his/mis/evc
result readTest(FILE *tracefile, entrySet cache, int s, int S, int E, int b, int verbose) {
    result Res = {0, 0, 0};

    char op;
    unsigned int address;
    // ignore size
    int size;

    while ((fscanf(tracefile, " %c %x,%d", &op, &address, &size)) > 0) {
        // ignore Instruction load
        if (op == 'I')
            continue;
        else {
            int indexSet_mask = (1 << s) - 1;
            int indexSet = (address >> b) & indexSet_mask;
            int tag = (address >> b) >> s;

            entryLine tgtLine = cache[indexSet];

            if (op == 'L' || op == 'S') {
                if (verbose) printf("%c %x", op, address);
                Res = HME(tgtLine, Res, E, tag, verbose);
            }

            // consider M as L then S
            else if (op == 'M') {
                if (verbose) printf("%c %x", op, address);

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
    int s = 0;        // number of sets ndex's bits
    int S = 0;        // number of sets
    int E = 0;        // number of lines
    int b = 0;        // number of blocks index's bits

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
                if (atoi(optarg) <= 0) {
                    printf("%s", help_message);
                    exit(0);
                }
                // s is bits and S = 2^s = 1<<s
                s = atoi(optarg);
                S = 1 << s;
            }

            // E for num of lins in a set
            case 'E': {
                if (atoi(optarg) <= 0) {
                    printf("%s", help_message);
                    exit(0);
                }
                E = atoi(optarg);
                break;
            }

            // b for number of bytes in a DRAM section
            case 'b': {
                if (atoi(optarg) <= 0) {
                    printf("%s", help_message);
                    exit(0);
                }
                b = atoi(optarg);
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

    // crate cache, test and get result into result struct
    cache = initializeCache(S, E);

    Result = readTest(tracefile, cache, s, S, E, b, verbose);

    realseMem(cache, S, E);

    printSummary(Result.hit, Result.miss, Result.evic);
    return 0;
}
