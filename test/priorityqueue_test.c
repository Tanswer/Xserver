/*
 * @filename:    priorityqueue_test.c
 * @author:      Tanswer
 * @date:        2017年12月24日 14:54:23
 * @description:
 */

#include <stdio.h>
#include <stdlib.h>
#include "../src/priorityqueue.h"
#include "../src/dbg.h"

static int comp(void *i, void *j)
{
    size_t si = (size_t)i;
    size_t sj = (size_t)j;

    return (si < sj) ? 1 : 0;
}

int testdata[] = {2,5,1,6,12,67,34,13,76,234,87 };

int main()
{
    xm_pq_t pq;
    int rc;
    rc = xm_pq_init(&pq, comp, 10);
    check_exit(rc == 0,"mx_pq_init error");

    rc = xm_pq_is_empty(&pq);
    check_exit(rc == 1, "xm_pq_is_empty error");

    size_t size;
    size = xm_pq_size(&pq);
    check_exit(size == 0, "xm_pq_size");

    void *min;
    min = xm_pq_min(&pq);
    check_exit(min == NULL, "xm_pq_min");

    rc = xm_pq_delmin(&pq);
    check_exit(rc == 0, "xm_pq_delmin error");

    int n = sizeof(testdata)/sizeof(int);
    int i;
    for(i = 0; i<n; i++){
        rc = xm_pq_insert(&pq, (void *)testdata[i]);
        check_exit(rc == 0, "xm_pq_insert error");
        
        check_exit(xm_pq_size(&pq) == (size_t)i+1, "xm_pq_size error");
    }
    
    printf("size = %ld  , capacity = %ld\n", pq.size, pq.capacity);
    
    while(!xm_pq_is_empty(&pq)){
        min = xm_pq_min(&pq);
        check_exit(min != NULL, "xm_pq_min error");
        printf("%d\n",(int)min);

        rc = xm_pq_delmin(&pq);
        check_exit(rc == 0, "xm_pq_delmin error");
    
        printf("size = %ld  , capacity = %ld\n", pq.size, pq.capacity);
    }
    return 0;
}
