/*
 * @filename:    xm_main.c
 * @author:      Tanswer
 * @date:        2017年12月20日 11:28:44
 * @description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "xm_http.h"
#include "xm_http_request.h"
#include "dbg.h"
#include "threadpool.h"
#include "util.h"
#include "timer.h"
#include "epoll.h"

#define CONF "xm.conf"

extern struct epoll_event *events;

int main(int argc, char *argv[])
{
    int rc;     //保存返回结果
    int port = 5000;
    int lfd;
    char *conf = CONF;

    /*
     * read confile file
     */
    char conf_buf[BUFLEN];
    xm_conf_t cf;
    rc = read_conf(conf, &cf, conf_buf, BUFLEN);
    check(rc == XM_CONF_OK, "read_conf");

    struct sockaddr_in clientaddr;
    memset(&clientaddr, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(clientaddr);

    lfd = open_listenfd(port);
    rc = setnonblocking(lfd);
    check(rc == 0,"setnonblocking");

    int epfd = xm_epoll_create(0);
    struct epoll_event event;

    xm_http_request_t *request = (xm_http_request_t *)malloc(sizeof(xm_http_request_t));
    xm_init_request(request, lfd, epfd, &cf);

    event.data.ptr = (void *)request;
    event.events = EPOLLIN | EPOLLET;
    xm_epoll_add(epfd, lfd, &event);

    int i, n, fd;

    while(1){
        n = xm_epoll_wait(epfd, events, MAXEVENTS, 0);
        
        for(i = 0; i<n; i++){
            xm_http_request_t *r = (xm_http_request_t *)events[i].data.ptr;
            fd = r -> sockfd;

            if(fd == lfd){
                /* yet have one or more connections */
                int connfd;
                while(1){
                    connfd = accept(lfd, (struct sockaddr *)&clientaddr, &len);
                    if(connfd < 0){
                        if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
                            break;      //done
                        }else{
                            log_err("accept");
                            break;
                        }
                    }

                    rc = setnonblocking(connfd);
                    check(rc == 0, "setnonblocking");
                    log_info("new connection fd %d",connfd);

                    xm_http_request_t *request = (xm_http_request_t *)malloc(sizeof(xm_http_request_t));
                    if(request == NULL){
                        log_err("malloc(sizeof(xm_http_request_t))");
                        break;
                    }

                    xm_init_request(request, connfd, epfd, &cf);
                    event.data.ptr = (void *)request;
                    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

                    xm_epoll_add(epfd, connfd, &event);

                } //end of while of accept 
            } else {
                if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN))) {
                    log_err("epoll error fd: %d", fd);
                    close(fd);
                    continue;
                }

                log_info("new data from fd %d",fd);

                do_request(events[i].data.ptr);
            }
        }
    }

    return 0;
}
    






