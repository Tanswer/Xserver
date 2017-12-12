/*
 * @filename:    main.c
 * @author:      Tanswer
 * @date:        2017年12月11日 15:55:14
 * @description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "util.h"
#include "epoll.h"
#include "threadpool.h"
#include "http.h"

extern struct epoll_event events;

int main(int argc, char *argv[])
{
    
    /* initialize listening socket */
    int lfd;

    lfd = open_listenfd(cf.port);
    rc = setnonblocking(lfd);
    check(rc == 0, "setnonblocking");


    /* create epoll and add lfd to ep */
    int epfd = mm_epoll_create(0);
    struct epoll_event event;
    
    mm_http_request_t *request = (mm_http_request_t *)malloc(sizeof(mm_http_request_t));
    mm_init_request(request, lfd, epfd, &cf);
    
    event.data.ptr = (void *)request;
    event.events = EPOLLIN | EPOLLET;
    mm_epoll_add(epfd, lfd, &event);

    
    int n;
    int fd;
    while(1){
        //time = mm_find_timer();
        n = mm_epoll_wait(epfd, events, MAXEVENTS, time);
        
        for(int i =0; i<n; i++){
            
            mm_http_request_t *r = (mm_http_request_t *)events[i].data.ptr;
            fd = r -> fd;

            
            if( lfd == fd ){
                while(1){
                    struct sockaddr_in clientaddr;
                    memset(&clientaddr, 0, sizeof(struct sockaddr_in));
                    socklen_t clientaddrlen = sizeof(clientaddr);
                    int connfd = accept(lfd, (struct sockaddr*)&clientaddr, &clientaddrlen);    
                    
                    if(connfd < 0){
                        if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
                            break;
                        }else {
                            break;   
                        }
                    }

                    rc = setnonblocking(connfd);
                    check(rc == 0,"setnonblocking");

                    mm_http_request_t * request = (mm_http_request_t *)malloc(sizeof(mm_http_request_t));
                    mm_init_request(request, connfd, epfd, &cf);
                    event.data.ptr = (void *)request;
                    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

                    mm_epoll_add(epfd, connfd, &event);
                }
            } else {
                if((events[i].events & EPOLLERR) || 
                        (events[i].events & EPOLLHUP) ||
                        (!(events[i].events & EPOLLIN))){
                    close(fd);
                    continue;
                }

                do_request(events[i].data.ptr);
            }
        }//end of for 
    }//end of while(1)
    
    
    
    return 0;
}
