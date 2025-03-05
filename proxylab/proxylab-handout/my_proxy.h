#ifndef _my_proxy_h
#define _my_proxy_h
#define _CRT_SECURE_NO_DEPRECATE 
#include "csapp.h"
#include <stdio.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
typedef struct obj{
    char request[MAXLINE];
    char respond[MAX_OBJECT_SIZE];
    int URL;
    int respond_size;
}obj; 

struct obj cache[9];
int readcnt;
sem_t mutex,w;


static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
void* thread(void* vargp);
void clienterror(int fd,char* cause,char* errnum,
                char* shortmsg, char* longmsg);
void do_it_go(int fd);
void prase(char* uri,char* host_name,char* post,
            char*filename);
void read_requesthdrs(rio_t* rp,char* Host);
int read_cache(char* requests,int fd);
void write_cache(char* requests,char* responds,int cont);
#endif
