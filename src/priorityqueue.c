/*
 * @filename:    priorityqueue.c
 * @author:      Tanswer
 * @date:        2017年12月24日 11:32:40
 * @description:
 */

#include "util.h"
#include "dbg.h"
#include "priorityqueue.h"

int xm_pq_init(xm_pq_t *pq, xm_pq_comparator_pt _comp, size_t _capacity)
{
    /* 从 1 开始，没用 0*/
    pq -> pq = (void **)malloc(sizeof(void *) * (_capacity+1));
    if(!pq -> pq){
        log_err("xm_pq_init:malloc failed");
        return -1;
    }

    pq -> size = 0;
    pq -> comp = _comp;
    pq -> capacity = _capacity + 1;


    return 0;
}

int xm_pq_is_empty(xm_pq_t *pq)
{
    return (pq -> size == 0) ? 1 : 0;
}


size_t xm_pq_size(xm_pq_t *pq)
{
    return pq -> size;
}

void *xm_pq_min(xm_pq_t *pq)
{
    if(xm_pq_is_empty(pq)){
        return NULL;
    }

    return pq -> pq[1];
}


static int resize(xm_pq_t *pq, size_t _newsize)
{
    if(_newsize <= pq -> size){
        log_err("resize:newsize is too small");
        return -1;
    }

    void **newptr = (void **)malloc(sizeof(void *) * _newsize);
    if(!newptr){
        log_err("resize:malloc error");
        return -1;
    }

    memcpy(newptr, pq -> pq, sizeof(void *) * (pq -> size + 1));
    free(pq->pq);
    pq -> pq = newptr;
    pq -> capacity = _newsize;

    return 0;
}

static void swap(xm_pq_t *pq, size_t i, size_t j)
{
    void *tmp;
    tmp = pq->pq[i];
    pq -> pq[i] = pq -> pq[j];
    pq -> pq[j] = tmp;
}

/* 插入元素时，如果孩子节点优先级大于其父节点 要进行上滤操作 */
static void swim(xm_pq_t *pq, size_t k)
{
    while(k > 1 && pq -> comp(pq -> pq[k], pq -> pq[k/2])){
        swap(pq, k, k/2);
        k /= 2;
    }
}

/* 删除根后，将最后一个元素交换到根，再从上往下调整堆 */
static size_t sink(xm_pq_t *pq, size_t k)
{
    size_t j;
    size_t size = pq -> size;

    while(2*k <= size){
        j = 2*k;    //左孩子
        /* 选出孩子中优先级高的 */
        if(j < size && pq -> comp(pq -> pq[j+1], pq -> pq[j]))  j++;
        /* 比较父节点与优先级较高的孩子节点的优先级 */
        if(!pq -> comp(pq -> pq[j],pq -> pq[k]))    break;
        /* 如果父节点的优先级比优先级较高的孩子节点的优先级还高，则直接跳出循环，否则交换 */
        swap(pq, j, k);
        
        k = j;
    }

    return k;
}

int xm_pq_delmin(xm_pq_t *pq)
{
    if(xm_pq_is_empty(pq)){
        return 0;
    }

    swap(pq, 1, pq -> size);

    pq -> size --;
    sink(pq, 1);

    if(pq -> size > 0 && pq -> size <= (pq -> capacity -1)/4 ){
        if(resize(pq, pq -> capacity / 2) < 0)
            return -1;
    }

    return 0;
}


int xm_pq_insert(xm_pq_t *pq, void *item)
{
    if(pq->size + 1 == pq -> capacity){
        if(resize(pq, pq -> capacity * 2) < 0)
            return  -1;
    }
    pq -> pq[++pq -> size] = item;
    swim(pq, pq -> size);

    return 0;
}


