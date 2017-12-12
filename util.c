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
#include <unistd.h>
#include "util.h"

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
