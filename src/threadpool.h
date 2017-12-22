#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include "dbg.h"


typedef struct xm_task_s
{
    void *(*task_callback)(void *); //任务的回调函数
    void *arg;                      //回调函数参数
    struct xm_task_s *next;         //下一个任务
}xm_task_t;

//线程池管理结构
typedef struct xm_threadpool_s
{
    int thread_num;                 //线程个数
    int free_thread_num;            //空闲线程个数
    int queue_max_num;              //最大任务个数
    int queue_curr_num;             //当前任务个数
    int resize_now;                 //是否调整过尺寸

    xm_task_t *head;                //任务队列头指针
    xm_task_t *tail;                //任务队列尾指针

    pthread_t *threads;             //所有线程的pthread_t
    pthread_mutex_t mutex;          //互斥信号量
    pthread_cond_t queue_empty;     //队列为空的信号量
    pthread_cond_t queue_not_empty; //队列非空的信号量
    pthread_cond_t queue_not_full;  //队列未满的信号量 

    int queue_close;                //任务队列是否关闭
    int pool_close;                 //线程池是否关闭
    
}xm_threadpool_t;

//初始化线程池
xm_threadpool_t *threadpool_init(int thread_num, int queue_max_num);

//调整线程池大小
int threadpool_resize(xm_threadpool_t *threadpool, int thread_num, int queue_max_num);

//销毁线程池
int threadpool_destroy(xm_threadpool_t *threadpool);

//向线程池添加任务
int threadpool_add_task(xm_threadpool_t *threadpool,void *(task_callback)(void *), void *arg, int is_block);

//线程的处理函数
void *thread_function(void *arg);


#endif
