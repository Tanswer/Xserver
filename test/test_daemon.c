/*
 * @filename:    test_daemon.c
 * @author:      Tanswer
 * @date:        2018年03月08日 09:35:19
 * @description:
 */

#include<stdio.h>
#include<stdlib.h>
#include "daemon.h"

int main()
{
    int fd;
    pid_t pid;

    Init_daemon();
    fd = open("/tmp/log", O_RDWR | O_TRUNC);
    if(fd == -1){
        perror("open");
    }

    pid = fork();

    if( pid > 0 ){
        for(int i=0; i<60;i++){
            write(fd, "hello world\n",12);
            sleep(1);
        }
    }
    close(fd);
    return 0;
}
