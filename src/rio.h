#ifndef RIO_H
#define RIO_H

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#define RIO_BUFSIZE 8192

typedef struct {
    int rio_fd;                 //内部读缓冲区描述符
    int rio_cnt;                //内部缓冲区中未读字节数
    char *rio_bufptr;           //内部缓冲区中下一个未读的字节
    char rio_buf[RIO_BUFSIZE];  //内部读缓冲区
}rio_t;     //一个类型为rio_t的读缓冲区


//RIO 无缓冲的I/O函数
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);

//带缓冲的输入函数
void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n);
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);


#endif
