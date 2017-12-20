#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>

#define MAXEVENTS 1024

int xm_epoll_create(int flags);
void xm_epoll_add(int epfd, int fs, struct epoll_event *event);
void xm_epoll_mod(int epfd, int fs, struct epoll_event *event);
void xm_epoll_del(int epfd, int fs, struct epoll_event *event);
int xm_epoll_wait(int epfd, struct epoll_event *event, int maxevents, int timeout);



#endif
