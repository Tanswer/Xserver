/*
 * @filename:    util.c
 * @author:      Tanswer
 * @date:        2017年11月17日 15:09:42
 * @description:
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "util.h"
#include "cJSON.h"
#include "dbg.h"

int open_listenfd(int port)
{
    int lfd, optval = 1;
    struct sockaddr_in serveraddr;

    if((lfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
        return -1;
    }

    if(setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR,
                (const void *)&optval, sizeof(int)) < 0)
        return -1;
    
    bzero((char *)&serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);

    if(bind(lfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr)) < 0)
        return -1;

    if(listen(lfd, LISTENMAX) < 0)
        return -1;

    return lfd;
}

//设置非阻塞
//成功返回0，失败返回-1
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    if(old_option == -1)
        return -1;
    int new_option = old_option | O_NONBLOCK;
    int res = fcntl(fd, F_SETFL, new_option);
    if(res == -1)
        return -1;
    return 0;
}


int read_conf(char *filename, xm_conf_t *cf)
{

    struct stat sbuf;
    if(stat(filename, &sbuf) < 0){
        log_err("stat error");
    }

    int srcfd = open(filename, O_RDONLY, 0);
    

    char *srcaddr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
    check(srcaddr != (void *)-1, "mmap error");
    close(srcfd);

    //debug("srcaddr = %s",srcaddr);

    cJSON* pJson = cJSON_Parse(srcaddr);
    check(pJson != NULL,"cJSON_Parse");

    cJSON *pSub;

    /* get root */
    pSub = cJSON_GetObjectItem(pJson, "root");
    check(pSub != NULL, "cJSON_GetObjectItem");
    cf -> root = (char *)pSub->valuestring;

    /* get port */
    pSub = cJSON_GetObjectItem(pJson, "port");
    check(pSub != NULL, "cJSON_GetObjectItem");
    cf -> port = pSub->valueint;

    /* get threadnum */
    pSub = cJSON_GetObjectItem(pJson, "threadnum");
    check(pSub != NULL, "cJSON_GetObjectItem");
    cf -> threadnum = pSub->valueint;


    /* get queuemaxnum */
    pSub = cJSON_GetObjectItem(pJson, "queuemaxnum");
    check(pSub != NULL, "cJSON_GetObjectItem");
    cf -> queuemaxnum = pSub->valueint;

    munmap(srcaddr, sbuf.st_size);

    return XM_CONF_OK;
}
