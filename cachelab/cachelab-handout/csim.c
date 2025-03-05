#include "cachelab.h"
#include "math.h"
#include <stdlib.h>
#include "stdio.h"
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>

int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;

typedef struct{
    unsigned tag;
    int LURcount;
    bool valid_bit;
}cache_line;

// void address_analysis(unsigned address,unsigned *tag,int *setsin,int *bbsi ){


// }
void time_add(int S,int E,cache_line **cache_sets){
    for(int i = 0; i < S;i++){
        for(int n = 0; n < E; n++){
            cache_sets[i][n].LURcount++;
        }
    }

}
void update(int s,int E,int b,int S, cache_line** cache_sets
            ,unsigned address){
        
        unsigned tag_1 = address>>(s+b);
        unsigned setin = (address>>b)^(tag_1<<s);

        for(int i = 0; i < E;i++){
            if(cache_sets[setin][i].tag == tag_1 &&cache_sets[setin][i].valid_bit == 1){
                hit_count++;
                cache_sets[setin][i].LURcount = 0;
                return;
            }
        }
        miss_count++;

        for(int i = 0; i < E; i++){
            if(cache_sets[setin][i].valid_bit == 0){
                cache_sets[setin][i].valid_bit = 1;
                cache_sets[setin][i].tag = tag_1;
                cache_sets[setin][i].LURcount = 0;
                return;
            }            
        }

        eviction_count++;
        int ee_cont = 0;
        int time_max = -1;
        
        for(int i = 0; i < E; i++){
           if(cache_sets[setin][i].LURcount > time_max){
                ee_cont = i;
                time_max = cache_sets[setin][i].LURcount;
           } 
        }
        cache_sets[setin][ee_cont].valid_bit = 1;
        cache_sets[setin][ee_cont].tag = tag_1;
        cache_sets[setin][ee_cont].LURcount = 0;
        return; 



}
void valg_solve_v0(int s,int S,int E,int b,
                    cache_line** cache_sets,char* tracename){
    //(void) s,E,b,hit_count,miss_count,evict_count,cache_sets;
    //printf("name: %s",tracename);
  
    FILE*pFile;	
    pFile = fopen(tracename, "r");	
    double S_ = pow(2,s);
    S = (int)S_;
    if(!pFile){
        printf("追踪文件呢？\n");
    }
    char identifier;	
    unsigned address;	
    int size;	

    printf("tttt\n");
    while(fscanf(pFile," %c %x,%i",&identifier,&address,&size)>0){
        switch(identifier){ 
            case 'L':
                update(s,E,b,s,cache_sets,address);
                break;
            case 'M':
                update(s,E,b,S,cache_sets,address);
                hit_count++;
                break;
            case 'S':
                update(s,E,b,S,cache_sets,address);
                break;
            }
        time_add(S,E,cache_sets);
        
    }
        printf("tt2\n");
        fclose(pFile);
       return; 
    }

                    

void print_help(){
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("-h         Print this help message.\n");
    printf("-v         Optional verbose flag.\n");
    printf("-s <num>   Number of set index bits.\n");
    printf("-E <num>   Number of lines per set.\n");
    printf("-b <num>   Number of block offset bits.\n");
    printf("-t <file>  Trace file.\n");
    printf("Examples:\n");
    printf("linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

cache_line** creat_cache(int s,int S, int E,int b){
    cache_line**cache_sets = (cache_line**)malloc(S*(sizeof(cache_line*)));
    if(cache_sets == NULL) return NULL;
    for(int i = 0; i < S; i++){
        cache_sets[i] = (cache_line*)malloc(E*(sizeof(cache_line)));
    }
    for(int i = 0; i < S; i++){
        for(int m = 0; m < E; m++){
            cache_sets[i][m].valid_bit = 0;
            cache_sets[i][m].LURcount = 0;
            cache_sets[i][m].tag = 0;
        }
    }
    printf("ee\n");
    return cache_sets;
}

int main(int argc, char*argv[])
{   //r -s 6 -E 4 -b 5 -t traces/dave.trace
    int visable_choice = 0;
    int s = 0,E = 0,b = 0,S = 0;
    char ch;
    char* tracename;
    while((ch = getopt(argc,argv,"s:E:b:t:")) != -1){
        switch (ch)
        {
        // case 'h':
        //     print_help();
        //     return 1;
        //     break;
        // case 'v':
        //     visable_choice = 1;
        //     break;
        case 's':
            s = atoi(optarg);
            S = 1<<s;
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            tracename = (char*)optarg;
            break;
        default:
            printf("什么鬼参数\n");
            break;
        }
    }
    printf("%i %i %i %i %s\n",s,S,E,b,tracename);
    
    cache_line** cache_sets = creat_cache(s,S,E,b);
    if(!cache_sets){
        printf("malloc。。血压上升了没\n");
        return 2;
    }
    

//处理输入
        printf("val\n");
        valg_solve_v0(s,S,E,b,cache_sets,tracename);
    
    for(int i = 0; i < S ; i++){       
        free(cache_sets[i]);
    }
    free(cache_sets);

    
    printf("%i %i %i  %i\n",s,E,b,visable_choice);
    printSummary(hit_count,miss_count,eviction_count);
    return 0;
}
