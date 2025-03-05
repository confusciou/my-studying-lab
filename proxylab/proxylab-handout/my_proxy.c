#include "my_proxy.h"
#define _CRT_SECURE_NO_DEPRECATE 
#include <stdio.h>
#include <strings.h>

void *thread(void* vargp){
    int fd = *((int*)vargp);
    Pthread_detach(pthread_self());
     printf("test11 tread %d\n",fd);
    do_it_go(fd);
    Close(fd);
    return NULL;
}
void do_it_go(int fd){
    char buf[MAXLINE],method[MAXLINE],host_uri[MAXLINE],version[MAXLINE];
    char host[MAXLINE],port[MAXLINE],filename[MAXLINE];
    char Host[MAXLINE];
    // char filename[MAXLINE],cgiargs[MAXLINE];
    rio_t rio;

    char request[MAXLINE];
    
    Rio_readinitb(&rio,fd);
    Rio_readlineb(&rio,buf,MAXLINE);
    printf("Request headers:\n");
    printf("%s",buf);
    sscanf(buf,"%s %s %s",method,host_uri,version);
    if(strcasecmp(method,"GET")/*||strcasecmp(method,"POST")*/){
        //post还不会写
        clienterror(fd,method,"501","Not implemented","Tiny don't implement that method");
        return;
    }
    printf("\ntest2\n");
    // printf("%s\n",host_uri);
    prase(host_uri,host,port,filename);
    read_requesthdrs(&rio,Host);

    
    //准备转发的请求报头
    sprintf(request,"%s %s HTTP/1.0\r\n%s",method,filename,Host);
    sprintf(request,"%s%s",request,user_agent_hdr);
    sprintf(request,"%sConnection: close\r\n",request);
    sprintf(request,"%sProxy-Connection: close\r\n\r\n",request);

    // printf("test08 \n%s \n",request);
    // printf("test09 %s %s \n",host,port);
    printf("test4 ready to proxy\n");
    
    //缓存
    int cachenum = read_cache(request,fd);
    printf(" ans: %d\n",cachenum);
    if(cachenum != -1){
        // Rio_writen(fd,respond,sizeof(respond));
        write_cache(NULL,NULL,cachenum);
        return;
    }
// 连接目标服务器
    // char respon[MAXLINE];
    int clientfd = Open_clientfd(host,port);

    printf("test11 %d\n",clientfd);
    //rio包咋用来着
    rio_t retp;
    Rio_readinitb(&retp,clientfd);
    Rio_writen(clientfd,request,strlen(request));

   
    size_t n;
    int objsize = 0;
    char respond[MAX_OBJECT_SIZE];
    memset(respond, 0, sizeof(respond));
    printf("test 09\n");
    //回复给客户端


    //差这个？
    while ((n = Rio_readlineb(&retp, buf, MAXLINE)) != 0)
    {
         printf("proxy received %d bytes,then send\n", (int)n);
        Rio_writen(fd, buf, n);
        // sprintf(respond,"%s%s",respond,buf);
        objsize+=n;
        if(objsize<MAX_OBJECT_SIZE){
            size_t space_left = MAX_OBJECT_SIZE - objsize;
            strncat(respond, buf, space_left - 1); 
        }
    }
        printf("\n test 12 %s\n",respond);
    Close(clientfd);

    if(objsize < MAX_OBJECT_SIZE){
        write_cache(request,respond,-1);
    }

    return;
}

void prase(char* uri,char* host_name,char* post,
            char*filename){
    char* ptr1;
    char* ptr2;

    ptr1 = strstr(uri,"//");
    if(ptr1 == NULL){
        // printf("test07\n");
        strcpy(host_name,"localhost");
        strcpy(post,"80");
        strcpy(filename,"./tiny/home.html");
        return;
    }
    ptr1 = ptr1 + 2;
    // printf("\ntest1 \n%s\n",ptr1);
    ptr2 = strstr(ptr1,":");
    if(ptr2 != NULL){
        //uri内提供端口
        *ptr2 = '\0';
        strcpy(host_name,ptr1);
        ptr1 = ptr2+1;
        ptr2 = strstr(ptr1,"/");
        *ptr2 = '\0';
        strcpy(post,ptr1);
        *ptr2 = '/';
        ptr1 = ptr2;
        
    }
    else{
        //默认端口
        ptr2 = strstr(ptr1,"/");
        *ptr2 = '\0';
        strcpy(host_name,ptr1);
        strcpy(post,"80");
        *ptr2 = '/';
        ptr1 = ptr2;
   }
   strcpy(filename,ptr1);
//    printf("test2  [%s]  [%s] [%s]\n",host_name,post,filename);

    return;
}

void clienterror(int fd,char* cause,char* errnum,
                char* shortmsg, char* longmsg){
    //准备报头
    char buf[MAXLINE],body[MAXLINE];
    sprintf(body,"<html><title>Tiny Error</title>");
    sprintf(body,"%s<body cgcolor=""fffff"">\r\n",body);
    sprintf(body,"%s%s: %s\r\n",body,longmsg,shortmsg);
    sprintf(body,"%s<p>%s:%s\r\n",body,longmsg,cause);
    sprintf(body,"%s<hr><em>The Tiny Web server</em>\r\n",body);

    //输出http响应
    sprintf(buf,"HTTP/1.0 %s %s\r\n",errnum,shortmsg);
    Rio_writen(fd,buf,strlen(buf));
    sprintf(buf,"Content_type: text/html\r\n");
    Rio_writen(fd,buf,strlen(buf));
    sprintf(buf,"Content_length:%d\r\n\r\n",(int)strlen(body));
    Rio_writen(fd,buf,strlen(buf));
    Rio_writen(fd,body,strlen(body));
}


void read_requesthdrs(rio_t* rp,char* Host){
    char buf[MAXLINE];
    Rio_readlineb(rp,buf,MAXLINE);
    sprintf(Host,"%s%s",Host,buf);
    while(strcmp(buf,"\r\n")){
        Rio_readlineb(rp,buf,MAXLINE);
    }
    printf("test04%s",Host);
    //按要求，至少要能解析出host
    return;
}
int read_cache(char* requests,int fd){
    printf("test05 reading");
    P(&mutex);
    readcnt++;
    if(readcnt == 1){
        P(&w);
    }
    V(&mutex);
    int i;
    int cont = -1;
    for(i= 0; i < 9; i++){
        if(cache[i].URL <= 0){
            continue;
        }
        if(strcmp(cache[i].request,requests) == 0){
            //已缓存
            printf("\ntest07\n%s\n",cache[i].respond);
            Rio_writen(fd,cache[i].respond,cache[i].respond_size);
            cont = i;
            break;
        }
    }
    P(&mutex);
    readcnt--;
    if(readcnt == 0){
        V(&w);
    }
    V(&mutex);
    return cont;
}

void write_cache(char* requests,char* responds,int cccin){
    printf("test06 wtring %d\n",cccin);
    if(cccin == -1){
        int i;
        int url = cache[0].URL;
        int cont = 0;
       for(i = 1;i < 9; i++){
           if(cache[i].URL < url ){
                cont = i;
                url = cache[i].URL;
           }
        }
        printf("wring into %d \n%s \n%s\n",cont,requests,responds);
        P(&w);
        cache[cont].URL = 1;
        cache[cont].respond_size = strlen(responds);
        strcpy(cache[cont].request,requests);
        strcpy(cache[cont].respond,responds);
       
    }
    else{
        printf("writing adding %d\n",cccin);
        P(&w);
        cache[cccin].URL++;
    }
    V(&w);
}