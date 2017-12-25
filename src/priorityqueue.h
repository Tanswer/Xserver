#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

#include <stddef.h>


#define XM_PQ_DEFAULT_CAPACITY  10

typedef int (*xm_pq_comparator_pt)(void *i, void *j);

typedef struct HeapStruct
{

    void **pq;                  //二叉堆数组
    size_t capacity;            //容量
    size_t size;                //实际元素个数
    xm_pq_comparator_pt comp;   //比较函数指针
}xm_pq_t; 


/* 初始化 */
int xm_pq_init(xm_pq_t *pq, xm_pq_comparator_pt _comp, size_t _capacity);
/* 判断队列是否为空 */
int xm_pq_is_empty(xm_pq_t *pq);
/* 取得队列中元素个数 */
size_t xm_pq_size(xm_pq_t *pq);

/* 取得优先级最高元素 */
void *xm_pq_min(xm_pq_t *pq);
/* 删除优先级最高元素 */
int xm_pq_delmin(xm_pq_t *pq);
/* 插入元素 */
int xm_pq_insert(xm_pq_t *pq, void *item);

//int xm_pq_sink(xm_pq_t *pq, size_t i);

#endif
