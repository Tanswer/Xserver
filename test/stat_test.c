/*
 * @filename:    test_stat.c
 * @author:      Tanswer
 * @date:        2017年12月20日 17:26:45
 * @description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>


int main()
{
    char filename[255] = "/home/Tanswer/Code/NP/http/html/index.html";

    struct stat sbuf;
    if(stat(filename, &sbuf) < 0)
    {
        printf("can not find the  file\n");
    }

    else{
        printf("hahaha\n");
        //printf("sbuf.st_mtime = %s\n",sbuf.st_mtime);
    }
    return 0;
}
