/*
 * @filename:    threadpool.c
 * @author:      Tanswer
 * @date:        2017年11月15日 10:35:09
 * @description: 线程池的实现
 */

#include "threadpool.h"
#include "dbg.h"

threadpool_t *threadpool_init(int thread_num, int queue_max_num)
{
    threadpool_t *threadpool = NULL;

    //放到一个while循环中???
    do{
        threadpool = (threadpool_t *)malloc(sizeof(threadpool_t));
        if(threadpool == NULL)
            break;
        //check(threadpool == NULL, "_Thread_local:malloc");
        
        threadpool -> thread_num = thread_num;
        threadpool -> free_thread_num = thread_num;
        threadpool -> queue_max_num = queue_max_num;
        threadpool -> queue_curr_num = 0;
        threadpool -> resize_now = 0;
        threadpool -> head = threadpool -> tail = NULL;
            
        if(pthread_mutex_init(&(threadpool -> mutex),NULL)){
            break;
        }
        if(pthread_cond_init(&(threadpool -> queue_empty),NULL)){
            break;
        }
        if(pthread_cond_init(&(threadpool -> queue_not_empty),NULL)){
            break;
        }
        if(pthread_cond_init(&(threadpool -> queue_not_full),NULL)){
            break;
        }

        threadpool -> threads = (pthread_t  *)malloc(sizeof(pthread_t)*thread_num);
        if(threadpool -> threads == NULL)
            break;

        threadpool -> queue_close = 0;
        threadpool -> pool_close = 0;

        //创建thread_num个线程
        int i;
        for(i=0; i<thread_num; i++){
            if(pthread_create(&(threadpool -> threads[i]),NULL,
                        thread_function,(void *)threadpool)){
                free(threadpool -> threads);
                break;
            }
        }
        return threadpool;
    }while(0);

    free(threadpool);
    return NULL;
}


//销毁线程池
int threadpool_destroy(threadpool_t * threadpool)
{
    //先上锁
    pthread_mutex_lock(&(threadpool -> mutex));

    //线程池已经退出了,就直接退出
    if(threadpool -> queue_close || threadpool -> pool_close ){
        pthread_mutex_unlock(&(threadpool -> mutex));
                return -1;
    }

    //置队列关闭标志
    threadpool -> queue_close = 1;

    //等待队列为空  注意这里的while
    while(threadpool -> queue_curr_num != 0){
        pthread_cond_wait(&(threadpool -> queue_empty), &(threadpool -> mutex));
    }

    //置线程池关闭标志
    threadpool -> pool_close = 1;
    pthread_mutex_unlock(&(threadpool -> mutex));

    //唤醒线程池中阻塞的线程 退出
    pthread_cond_broadcast(&(threadpool -> queue_not_empty));

    //唤醒阻塞的添加任务的函数 退出
    pthread_cond_broadcast(&(threadpool -> queue_not_full));


    //回收线程
    int i;
    for(i = 0; i<threadpool -> thread_num; i++){
        pthread_join(threadpool -> threads[i], NULL);
    }


    //清理资源
    pthread_mutex_destroy(&(threadpool -> mutex));
    pthread_cond_destroy(&(threadpool -> queue_not_full));
    pthread_cond_destroy(&(threadpool -> queue_not_empty));
    pthread_cond_destroy(&(threadpool -> queue_empty));
    free(threadpool -> threads);


    //清空任务队列
    task_t *free_task;
    while(threadpool -> head != NULL){
        free_task = threadpool -> head;
        threadpool -> head  = threadpool -> head -> next;
        free(free_task);
    }

    //释放线程池结构
    free(threadpool);

    return 0;
}

/* 
 * 调整线程池大小
 * return -1 参数错误不允许减小规模
 * return -2 申请内存错误
 * return -3 线程创建错误
 */
int threadpool_resize(threadpool_t *threadpool, int thread_num, int queue_num)
{
    //不允许减少规模
    if(thread_num < threadpool -> thread_num || queue_num < threadpool -> queue_max_num){
        return -1;
    }

    pthread_mutex_lock(&(threadpool -> mutex));
    threadpool -> queue_max_num = queue_num;

    threadpool -> threads = (pthread_t *)realloc(threadpool -> threads, sizeof(pthread_t)*thread_num);
    if(threadpool -> threads == NULL){
        return -2;
    }

    int i;
    for(i=threadpool -> thread_num; i<thread_num; i++){
        if(pthread_create(&(threadpool -> threads[i]),NULL,
                    thread_function,(void *)threadpool)){
            return -3;
        }
    }
    
    threadpool -> free_thread_num += (thread_num + threadpool -> thread_num);
    threadpool -> thread_num = thread_num;
    threadpool -> resize_now = 1;
    pthread_mutex_unlock(&(threadpool -> mutex));

    return 0;
}

/*
 * 向线程池添加任务
 * return  0 表示正常
 * return  1 表示任务队列已满或所有线程忙碌(仅当非阻塞模式)
 * return -1 表示其他错误
 */ 
int threadpool_add_task(threadpool_t *threadpool, void *(task_callback)(void *), void *arg, int is_block)
{
    pthread_mutex_lock(&(threadpool -> mutex));
    //队列满的时候
    while((threadpool -> queue_curr_num == threadpool -> thread_num) && 
            !(threadpool -> queue_close || threadpool -> pool_close)){
        if(is_block == 1){ //阻塞模式
            printf("threadpool -> queue_curr_num = %d\n",threadpool -> queue_curr_num);
            printf("threadpool -> queue_max_num = %d\n",threadpool -> queue_max_num);

            pthread_cond_wait(&(threadpool -> queue_not_full), &(threadpool -> mutex));
        } else {
            pthread_mutex_unlock(&(threadpool -> mutex));
            return 1;
        }
    }

    //所有线程都在忙的时候,且是 非阻塞 不再添加任务
    if(threadpool -> free_thread_num == 0 && is_block == 0){
        pthread_mutex_unlock(&(threadpool -> mutex));
        return 1;
    } 

    //队列关闭或者线程池关闭退出
    if(threadpool -> queue_close == 1 || threadpool -> pool_close == 1){
        pthread_mutex_unlock(&(threadpool -> mutex));
        return -1;
    }

    //正常的情况下
    task_t *new_task = (task_t *)malloc(sizeof(task_t));
    if(new_task == NULL){
        pthread_mutex_unlock(&(threadpool -> mutex));
        return -1;
    }

    new_task -> task_callback = task_callback;
    new_task -> arg = arg;
    new_task -> next = NULL;

    threadpool -> queue_curr_num++;
    if(threadpool -> head == NULL){
        threadpool -> head = threadpool -> tail = new_task;
        //告诉线程池中休眠的线程有活干了
        //broadcast会广播给所有等待任务的消费者，会产生惊群现象
        //pthread_cond_broadcast(&(threadpool -> queue_not_empty));
    } else {
        threadpool -> tail -> next = new_task;
        threadpool -> tail = new_task;
    }

    //添加一个任务发一个信号
    pthread_cond_signal(&(threadpool -> queue_not_empty));
    pthread_mutex_unlock(&(threadpool -> mutex));

    return 0;
}


//线程处理函数
void *thread_function(void *arg)
{
    threadpool_t *threadpool = (threadpool_t *)arg;
    task_t *task = NULL;
    while(1){
        pthread_mutex_lock(&(threadpool -> mutex));

        //等待队列不空的消息
        while((threadpool -> queue_curr_num == 0) && !threadpool -> pool_close ){
            pthread_cond_wait(&(threadpool -> queue_not_empty), &(threadpool -> mutex));
        }

        //线程池关闭时,线程退出
        if(threadpool -> pool_close){
            pthread_mutex_unlock(&(threadpool -> mutex));
            pthread_exit(NULL);
        }

        //拿取任务
        threadpool -> queue_curr_num--;
        task = threadpool  -> head;
        //取出任务后,任务队列为空了
        if(threadpool -> queue_curr_num == 0){
            threadpool -> head = threadpool -> tail = NULL;
            //发出任务队列为空的信号
            pthread_cond_signal(&(threadpool -> queue_empty));
        } else {
            threadpool -> head = threadpool -> head -> next;
        }

        //此时队列不空，发信号通知所有可能阻塞的任务添加函数可以工作了
        if(threadpool -> queue_curr_num == threadpool -> queue_max_num -1){
            pthread_cond_broadcast(&(threadpool -> queue_not_full));
        }

        threadpool -> free_thread_num--;
        pthread_mutex_unlock(&(threadpool -> mutex));
        
        
        //执行callback函数
        task -> task_callback(task -> arg);
        free(task);
        task = NULL;


        pthread_mutex_lock(&(threadpool -> mutex));
        threadpool -> free_thread_num++;
        pthread_mutex_unlock(&(threadpool -> mutex));
    }

}


// 线程池测试代码
void *work(void *arg)
{
	char *p = (char *)arg;
	printf("threadid = %u, thread_pool callback fuction : %s.\n", pthread_self(),p);

    //sleep(1);
}
int main(int argc, char** argv)
{
    return 0;
}

