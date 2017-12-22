/*
 * @filename:    threadpool_test.c
 * @author:      Tanswer
 * @date:        2017年12月22日 10:37:19
 * @description:
 */

#include<stdio.h>
#include<stdlib.h>

#include "../src/threadpool.h"

// 线程池测试代码
void *work(void *arg)
{
	char *p = (char *)arg;
	printf("thread_pool callback fuction : %s.\n", p);
	sleep(1);
}

int main()
{
	char *args[] = {
		"1",  "2",  "3",  "4",  "5",
		"6",  "7",  "8",  "9",  "10",
		"11", "12", "13", "14", "15",
		"16", "17", "18", "19", "20",
		"21", "22", "23", "24", "25",
		"26", "27", "28", "29", "30"};
	xm_threadpool_t *threadpool = threadpool_init(1, 2);
	int i = 0, tag, value;
	for (i = 0; i < 30; i++) {
		do {
			tag = threadpool_add_task(threadpool, work, args[i],0);
			if (tag == 1) {
				value = threadpool_resize(threadpool, 
					threadpool->thread_num * 2, threadpool->queue_max_num * 2);
				if (value == -1) {
					printf("参数错误!\n");
					exit(-1);
				} else if (value == -2) {
					printf("申请内存错误!\n");
					exit(-1);
				} else if (value == -3) {
					printf("线程创建错误!\n");
					exit(-1);
				}
			} //end of if
		}while (tag != 0);
	} // end if for
	sleep(2);
	threadpool_destroy(threadpool);
	return 0;
}
