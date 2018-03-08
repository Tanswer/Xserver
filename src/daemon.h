#ifndef DAEMON_H
#define DAEMON_H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/resource.h>

void Init_daemon()
{
    int fd;

    struct rlimit rl;
    
    if(fork() != 0){    // 第一次 fork
        exit(0);
    }

    if(setsid() == -1){ // 创建新会话
        perror("setsid");
    }

    if(fork() != 0){    // 第二次 fork
        exit(0);
    }

    if(chdir("/tmp") == -1){    // 修改默认工作记录
        perror("chdir");
    }

    umask(0);   // 设置权限掩码

    signal(SIGHUP, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    if(getrlimit(RLIMIT_NOFILE, &rl) < 0){  // 通过getrlimit 得到进程所能打开的最大文件描述符
        perror("getrlimit");
    }

    if(rl.rlim_max == RLIM_INFINITY){
        rl.rlim_max = 1024;
    }

    /* 将除过 0 1 2 之外的文件描述符全部关闭 */
    int i;
    for(i=3; i<rl.rlim_max; i++){
        close(i);
    }
    
    if((fd = open("/tmp/log", O_RDWR, 0) != -1)){
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);

        if(fd > STDERR_FILENO){
            close(fd);
        }
    }
}

#endif
