#ifndef TIMER_H
#define TIMER_H

#include "xm_http_request.h"
#include "priorityqueue.h"

#define TIMEOUT_DEFAULT     500     /* ms */
#define XM_TIMER_INFINITE   -1      /* 默认阻塞 */

typedef int (*timer_handler_pt)(xm_http_request_t *r);

typedef struct xm_timer_node_s {
    size_t key;                 //超时时间
    int deleted;                //是否删除
    timer_handler_pt handler;   //处理函数
    xm_http_request_t *r;
}xm_timer_node_t;


/* 初始化计时器，将会创建二叉堆 */
int xm_timer_init();            
/* 获取二叉堆中超时时间最小的一个节点 */
int xm_find_timer();            
/* 处理所有超时的事件 */
void xm_handle_expire_timers();  


/* 将事件加入二叉堆中 */
void xm_add_timer(xm_http_request_t *r, size_t timeout, timer_handler_pt handler);
/* 将某个事件删除 */
void xm_del_timer(xm_http_request_t *r);


#endif
