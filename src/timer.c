/*
 * @filename:    timer.c
 * @author:      Tanswer
 * @date:        2017年12月24日 22:55:53
 * @description:
 */

#include <sys/time.h>
#include <pthread.h>
#include "priorityqueue.h"
#include "timer.h"
#include "dbg.h"


static int timer_comp(void *i, void *j)
{
    xm_timer_node_t *timeri = (xm_timer_node_t *)i;
    xm_timer_node_t *timerj = (xm_timer_node_t *)j;

    return (timeri -> key < timerj -> key) ? 1 : 0;
}

/* 时间堆 */
xm_pq_t xm_timer;
/* 当前时间 */
size_t xm_current_msec;

pthread_mutex_t xm_time_lock;


/* 更新时间 */
static void xm_time_update()
{
    struct timeval tv;
    int rc;
    
    pthread_mutex_lock(&xm_time_lock);

    rc = gettimeofday(&tv, NULL);
    check(rc == 0, "xm_time_update:gettimeofday error");

    xm_current_msec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    debug("time = %zu",xm_current_msec );
    
    pthread_mutex_unlock(&xm_time_lock);
}


/* 初始化计时器，创建起一个二叉堆，来维护计时器 */
int xm_timer_init()
{
    int rc;

    /* 初始化二叉堆 */
    rc = xm_pq_init(&xm_timer, timer_comp, XM_PQ_DEFAULT_CAPACITY);
    check(rc == 0, "xm_pq_init error");

    xm_time_update();

    return XM_OK;
}

/* 
 * 找到当前二叉堆中超时时间最小的一个节点
 * 返回它还剩下的超时时间，为epoll 的 wait 提供一个超时时间 
 */
int xm_find_timer()
{
    xm_timer_node_t *timer_node;
    int time = XM_TIMER_INFINITE;
    int rc;

    while(!xm_pq_is_empty(&xm_timer)){
        debug("xm_find_timer");
        xm_time_update();

        /* 找到 key 最小的节点 */
        timer_node = (xm_timer_node_t *)xm_pq_min(&xm_timer);
        check(timer_node != NULL, "xm_pq_min error");

        if(timer_node -> deleted){
            rc = xm_pq_delmin(&xm_timer);
            check(rc == 0, "xm_pq_delmin");
            free(timer_node);
            continue;
        }

        time = (int)(timer_node -> key - xm_current_msec);
        debug("xm_find_timer, key = %zu, cur = %zu",timer_node -> key, xm_current_msec);

        time = time > 0 ? time : 0;
        break;
    }
    return time;    // 如果没有二叉堆为空 就返回 -1，epoll_wait 一直阻塞
}

/* 处理二叉堆中所有超时的事件 */
void xm_handle_expire_timers()
{
    debug("xm_handle_expire_timers");
    xm_timer_node_t *timer_node;
    int rc;

    /* 找到所有的超时事件，然后处理它们 */
    while(!xm_pq_is_empty(&xm_timer)){
        debug("xm_handle_expire_timers,size = %zu",xm_pq_size(&xm_timer));
        /* 更新时间 */
        xm_time_update();
        
        /* 找出最近的事件 */
        timer_node = (xm_timer_node_t *)xm_pq_min(&xm_timer);
        check(timer_node != NULL, "xm_pq_min error");

        /* 首先判断是否删除 */
        if(timer_node -> deleted){
            rc = xm_pq_delmin(&xm_timer);
            check(rc == 0, "xm_handle_expire_timers:xm_pq_delmin error");
            free(timer_node);
            continue;
        }
       
        /* 最近的是否超时，如果不，则剩下的都不超时，直接返回 */
        if(timer_node -> key > xm_current_msec)
            return ;

        /* 调用 handler 来处理这个事件 */
        if(timer_node -> handler)
            timer_node -> handler(timer_node -> r);

        /* 将此节点删除 */
        rc = xm_pq_delmin(&xm_timer);
        check(rc == 0, "xm_handle_expire_timers: xm_pq_delmin error");
        
        /* 回收空间 */
        free(timer_node);
    }
}


/* 将一个事件加入到二叉堆中，它的超时时间为 timeout */
void xm_add_timer(xm_http_request_t *r, size_t timeout, timer_handler_pt handler)
{
    int rc;

    /* 申请空间 */
    xm_timer_node_t *timer_node = (xm_timer_node_t *)malloc(sizeof(xm_timer_node_t));
    check(timer_node != NULL, "xm_add_timer:malloc error");

    /* 更新时间 */
    xm_time_update();

    r -> timer = timer_node;
    /* 表示该 event 的超时时间，为当前时间的值加上超时变量 */
    timer_node -> key = xm_current_msec + timeout;
    debug("xm_add_timer  key = %zu", timer_node -> key);

    timer_node -> deleted = 0;
    timer_node -> handler = handler;
    timer_node -> r = r;

    /* 将节点插入到二叉堆中 */
    rc = xm_pq_insert(&xm_timer, timer_node);
    check(rc == 0, "xm_add_timer: xm_pq_insert error");
}

/* 将某个事件删除 */
void xm_del_timer(xm_http_request_t *r)
{
    debug("xm_del_timer");
    xm_time_update();
    xm_timer_node_t *timer_node = r -> timer;
    check(timer_node != NULL, "xm_del_timer:r->timer is NULL");

    timer_node -> deleted = 1;
}

