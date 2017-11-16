/*
 * @filename:    epoll.c
 * @author:      Tanswer
 * @date:        2017年11月15日 09:02:07
 * @description:
 */


#include "epoll.h"
#include "dbg.h"


struct epoll_event *events;

int epoll_create(int flags)
{
    int fd = epoll_create1(flags);

    check(fd > 0, "epoll_create:epoll_create1");

    events = (struct epoll_event *)malloc(sizeof(struct epoll_event)*MAXEVENTS);
    check(events != NULL, "epoll_create:malloc");

    return fd;
}

void epoll_add(int epfd, int fd, struct epoll_event* event)
{
    int res = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, event);
    check(res == 0, "epoll_add:epoll_ctl");
    return ;
}

void epoll_mod(int epfd, int fd, struct epoll_event *event)
{
    int res = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, event);
    check(res == 0,"epoll_mod:epoll_ctl");
    return ;
}

void epoll_del(int epfd, int fd, struct epoll_event *event)
{
    int res = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, event);
    check(res == 0, "epoll_del:epoll_ctl");
    return ;
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
    int n = epoll_wait(epfd, events, maxevents, timeout);
    check(n >=0, "epoll_wait:epoll_wait");
    return n;
}
