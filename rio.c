/*
 * @filename:    rio.c
 * @author:      Tanswer
 * @date:        2017年11月16日 09:08:33
 * @description: RIO (Robust I/O) 函数的实现
 */


#include "rio.h"


/*
 * 无缓冲输入函数 
 * 成功返回收入的字节数 
 * 若EOF返回0 ，出错返回-1
 */
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
   size_t nleft = n;
   ssize_t nread = 0;
   char *pbuf = usrbuf;

   while(nleft > 0){
       //在某些系统中，当处理程序捕捉到一个信号时，被中断的系统调用(read write accept)
       //在信号处理程序返回时不再继续，而是立即返回给客户一个错误条件，并将errno设置成为EINTR
       if((nread = read(fd, pbuf,nleft)) == -1){
           if(errno == EINTR){  
               nread = 0;   //中断造成的，再次调用read
           } else{
               return -1;   //出错
           }
       }
       else if(nread == 0)  //到了文件末尾
           break;
       nleft -=nread;
       pbuf += nread;
   }
   return n-nleft;
}

/*
 * 无缓冲输出函数
 * 成功返回输出的字节数，出错返回-1
*/ 
ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while(nleft > 0){
        //这里是小于等于，磁盘已满或者超过一个给定进程的文件长度限制就出错了
        if((nwritten = write(fd, bufp, nleft )) <= 0)
        {
            if(errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -=nwritten;
        bufp += nwritten;
    }

    return n;
}

// 初始化rio_t结构，创建一个空的读缓冲区
// 将fd和地址rp处的这个读缓冲区联系起来
void rio_readinitb(rio_t *rp, int fd)
{
    rp -> rio_fd = fd;
    rp -> rio_cnt = 0;
    rp -> rio_bufptr = rp -> rio_buf;
}


//是 RIO 读程序的核心，是Unix read函数的带缓冲的版本
ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;
    while(rp -> rio_cnt <= 0){  //内部缓冲区空了
        rp -> rio_cnt = read(rp -> rio_fd, rp -> rio_buf, sizeof(rp -> rio_buf));
        if(rp -> rio_cnt < 0){
            if(errno != EINTR)  //出错返回
                return -1;
        } 
        else if(rp -> rio_cnt == 0)  //EOF
            return 0;
        else
            rp -> rio_bufptr = rp -> rio_buf;   //重置指针位置
    }

    //从内部缓冲区拷贝到用户缓冲区中
    cnt = (rp -> rio_cnt < n) ? rp -> rio_cnt : n;

    memcpy(usrbuf, rp -> rio_bufptr, cnt);

    rp -> rio_bufptr += cnt;
    rp -> rio_cnt -= cnt;

    return cnt;
}


ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int rc,n;
    char c,*bufp = usrbuf;
    for(n = 1; n<maxlen; n++){
        if((rc = rio_read(rp, &c, 1)) == 1){     //每次读取一个字符
            *bufp++ = c;
            if(c == '\n')
                break;
        } else if(rc == 0){
            if(n == 1)      //空文件
                return 0;
            else
                break;      //读到部分数据
        } else
            return -1;      //出错
    }

    *bufp = '\0';
    return n-1;
}


//带缓冲输入函数
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while(nleft > 0){
        if((nread = rio_read(rp, bufp, nleft)) < 0){
                return -1;      
        } else if( nread == 0 )
            break;      //EOF
        
        nleft -= nread;
        bufp += nread;
    }

    return (n - nleft);
}
