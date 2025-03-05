//重拾分文件编写,另外，要适应makefile
#define _CRT_SECURE_NO_DEPRECATE
#include "my_proxy.h"
#include "csapp.h"
#include <stdio.h>
#include <stdlib.h>
/* Recommended max cache and object sizes */


/* You won't lose style points for including this long line in your code */

//proxy

int main(int argc,char ** argv)
{
    //创建listedfd
    int listenfd,connfd,*t_fd;
    char client_hostname[MAXLINE],client_port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;


    Sem_init(&mutex,0,1);
    Sem_init(&w,0,1);
    readcnt = 0;
    for(int i =0; i< 9; i++){
        cache[i].URL = 0;
    }
    printf("\ntest00 inint\n");

    if(argc != 2){
        fprintf(stderr,"usage: %s <port>\n",argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd,(SA*)&clientaddr,&clientlen);
        t_fd = (int*)Malloc(sizeof(int));
        *t_fd = connfd;
        Getnameinfo((SA*)&clientaddr,clientlen,client_hostname,MAXLINE,
                    client_port,MAXLINE,0);
        printf("\nConnect to (%s,%s)\n",client_hostname,client_port);
        Pthread_create(&tid,NULL,thread,t_fd);
    }
    


//
    printf("%s", user_agent_hdr);

    return 0;
}
