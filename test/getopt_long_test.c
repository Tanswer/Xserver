/*
 * @filename:    getopt_long__test.c
 * @author:      Tanswer
 * @date:        2017年12月21日 10:16:33
 * @description:
 */

#include<stdio.h>
#include<stdlib.h>
#include <getopt.h>

static const  struct option long_options [] = {
    {"reqarg", required_argument, NULL, 'r'},
    {"noarg", no_argument, NULL, 'n'},
    {"optarg", optional_argument, NULL, 'o'},
    {0, 0, 0, 0}
};

int main(int argc, char *argv[])
{
    int opt;    //函数返回值
    char *optstring = "a:b:c:d";
    int option_index = 0;

    while((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1){
        printf("opt = %c\n",opt);
        printf("optarg = %s\n",optarg);
        printf("optind = %d\n",optind);
        printf("argv[optind - 1] = %s\n",argv[optind-1]);
        printf("option_index = %d\n",option_index);
    }
    
    return 0;
}
