#include "cachelab.h"
#include "getopt.h"
#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "errno.h"

//[set1(line1,lie2), set2(line...)]
typedef struct
{
    int valid;
    int tag;
    int accessCount
}line;

typedef line *entryLine;

typedef entryLine *entrySet;

//use a struct to save results
typedef struct 
{
    int hit; 
    int miss;
    int evic;
}result;





int main(int argc, char*const argv[])
{   
    //init result
    result Result = {0, 0, 0};

    const char *help_message = "Wrong Input!\n Usage: \"Your complied program\" [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n" \
                     "<s> <E> <b> should all above zero and below 64.\n" \
                     "Complied with std=c99\n";
    const char *command_options = "hvs:E:b:t:";

    //init trace and cache
    FILE* tracefile = NULL;
    entrySet cache = NULL;
    
    int verbose = 0;    //1 to switch to verbose mode, default 0
    int s = 0;          //number of sets ndex's bits
    int S = 0;          //number of sets
    int E = 0;          //number of lines
    int b = 0;          //number of blocks index's bits

    //get args
    char tmp;
    while ((tmp = getopt(argc, argv, command_options))!=-1)
    {
        switch(tmp){

            //h for help_msg and exit
            case 'h':{
                printf("%s",help_message);
                exit(1);
            }

            //v for verbose mode
            case 'v':{
                verbose = 1;
                break;
            }

            //analyse effective argument
            //s for bits of set_num
            case 's':{
                //assuming there's at least 2 sets(bit 1)
                if (atoi(optarg)<=0){
                    printf("%s",help_message);
                    exit(0);
                }
                //s is bits and S = 2^s = 1<<s
                s = atoi(optarg);
                S = 1 << s;
            }

            //E for num of lins in a set
            case 'E':{
                if (atoi(optarg)<=0){
                    printf("%s",help_message);
                    exit(0);
                }
                E = atoi(optarg);
                break;
            }

            //b for number of bytes in a DRAM section
            case 'b':{
                if (atoi(optarg)<=0){
                    printf("%s",help_message);
                    exit(0);
                }
                b = atoi(optarg);
                break;
            }

            //t's nxt argv signify trace filename
            case 't':{
                if ((tracefile = fopen(optarg, "r")) == NULL){
					printf("Failed to open tracefile");
					exit(0);
				}
				break;
            }

            default:{
                printf("%s",help_message);
                exit(0);
            }
        }
    }

    //crate cache, test and get result into result struct
    cache = InitializeCache(S, E);
    Result = ReadAndTest(tracefile, cache, S, E, s, b, verbose);
    RealseMemory(cache, S, E);

    printSummary(Result.hit, Result.miss, Result.evic);
    return 0;
}
