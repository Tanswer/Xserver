/*
 * @filename:    http.c
 * @author:      Tanswer
 * @date:        2017年12月11日 19:21:28
 * @description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include "dbg.h"
#include "epoll.h"
#include "xm_http_request.h"
#include "xm_http.h"
#include "rio.h"
#include "timer.h"
#include "dns.h"

/* 解析uri 得到域名 */
static void parse_uri(char *uri, int uri_length, char *domainname);
/* 请求回复 */
static void Response(int fd, char *ips);


void do_request(void *ptr)
{
    xm_http_request_t *r = (xm_http_request_t *)ptr;

    int fd = r -> sockfd;
    char domainname[SHORTLINE];
    memset(domainname, 0, strlen(domainname));
    char *plast = NULL;
    int rc, n;
    size_t remain_size;
    
    xm_del_timer(r);

    for(;;){ /* 循环读取客户数据并分析之 */
        plast = &r -> buf[r->last % MAX_BUF];
        remain_size = Min(MAX_BUF - (r->last - r->pos) - 1,
                MAX_BUF - r-> last % MAX_BUF);
        
        n = read(fd , plast, remain_size);
        check(r->last - r->pos < MAX_BUF, "request buffer overflow!");

        if(n == 0){
            /* 客户端退出 */
            log_info("read return 0,ready to close fd %d,remain_size = %zu",fd,remain_size);
            goto close;      
        }
        if(n < 0){
            if( errno != EAGAIN ){      /* 出错 */
                log_err("read error, errno  = %d",errno);

                /* 出错关闭 */
                goto err;
            }
            break;  /* EAGAIN */
        }

        r -> last += n;
        check(r->last - r->pos < MAX_BUF, "request buffer overflow!");
       
        if(r->STATE == 0)
        {
            log_info("ready to parse request line");
            rc = xm_http_parse_request_line(r);
            if(rc == XM_AGAIN){
                continue;
            }else if(rc != XM_OK){
                log_err("rc != XM_OK");
                goto err;
            }
        }
        
        
        log_info("method = %.*s",(int)(r->method_end - r->request_start + 1),(char *)r->request_start);
        log_info("uri = %.*s",(int)(r->uri_end - r->uri_start),(char *)r->uri_start);

        if(r->STATE == 1)
        {
            log_info("ready to parse header line");
            rc = xm_http_parse_header_line(r);
            if(rc == XM_AGAIN){
                continue;
            }else if(rc != XM_OK){
                log_err("rc != XM_OK");
                goto err;
            }
        }

        log_info("header line parse finished");

        
        parse_uri(r->uri_start, r->uri_end - r->uri_start,domainname);
        debug("domainname = %s",domainname);
        if(strlen(domainname) == 0)
            break;


        char ips[256];
        memset(ips, 0, strlen(ips));
        Resolve(domainname, ips);
        debug("ips = %s", ips);

        Response(fd, ips);
    } 

    struct epoll_event event;
    event.data.ptr = ptr;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

    xm_epoll_mod(r->epfd, r->sockfd, &event);
    xm_add_timer(r, TIMEOUT_DEFAULT, xm_http_close_conn);
    
    return ;

err:
close:
    rc = xm_http_close_conn(r);
    check(rc == 0, "do_request:xm_http_close_conn");

}


// 从 uri 解析出域名
static void parse_uri(char *uri, int uri_length, char *domainname)
{
    check(uri !=NULL, "uri is NULL");
    /* uri_length can not be too long */
    if(uri_length > (SHORTLINE >> 1)){
        log_err("uri too long: %.*s",uri_length, uri);
        return ;
    }
    uri[uri_length] = '\0';
    
    char *w = strstr(uri, "?dn=");
    if(w != NULL)
        strcpy(domainname, w+4);
    else
        domainname = NULL;
    
    log_info("domainname = %s",domainname);
    return ;
}


static void Response(int fd, char *ips)
{
    debug("start Response");
    char header[MAXLINE];
    size_t n;

    sprintf(header, "HTTP/1.1 200 OK");
    sprintf(header, "%sConnection: close\r\n",header);
    sprintf(header, "%sServer: Xserver\r\n",header);
    sprintf(header, "%sContent-type: text/html\r\n",header);
    sprintf(header, "%sContent-Lenght: %zu\r\n",header, strlen(ips));

    sprintf(header, "%s\r\n",header);
    /* header end */
    
    n = (size_t)rio_writen(fd, header, strlen(header));    
    check(n == strlen(header), "rio_writen error, errno = %d",errno);


    n = rio_writen(fd, ips, strlen(ips));
    check(n == strlen(ips), "rio_written error");
}

