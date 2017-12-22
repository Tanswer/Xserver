/*
 * @filename:    getopt_test.c
 * @author:      Tanswer
 * @date:        2017年12月21日 10:01:21
 * @description:
 */

/*
 * 函数原型
 * 
 * int getopt(int argc, char * const argv[],  
 *            const char *optstring);  
 *              
 * extern char *optarg;  
 * extern int optind, opterr, optopt;  
 * 返回值：
 * 返回指定的参数
 * optarg: 指定参数对应的值
 * optind: 下一个将被处理到的参数在 argv中的下标值
 */

#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int opt;
    char *optstring = "a:b:c:d";

    while((opt = getopt(argc, argv, optstring)) != -1){
        printf("opt = %c\n",opt);
        printf("optarg = %s\n",optarg);
        printf("optind = %d\n",optind);
        printf("argv[optind-1] = %s\n",argv[optind-1]);
    }
    return 0;
}

