/*
 * @filename:    xm_http_request.c
 * @author:      Tanswer
 * @date:        2017年12月13日 17:47:58
 * @description:
 */

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "xm_http_request.h"
#include "list.h"

#define _XOPEN_SOURCE

static int xm_http_process_ignore(xm_http_request_t *r, xm_http_out_t *out, char *data, int len);
static int xm_http_process_connection(xm_http_request_t *r, xm_http_out_t *out, char *data, int len);
static int xm_http_process_if_modified_since(xm_http_request_t *r, xm_http_out_t *out, char *data, int len);

xm_http_header_handle_t xm_http_headers_in [] = {
    {"Host", xm_http_process_ignore},
    {"Connection", xm_http_process_connection},
    {"If-Modified-Since", xm_http_process_if_modified_since},
    {"", xm_http_process_ignore}
};


int xm_init_request(xm_http_request_t *r, int fd, int epfd, xm_conf_t *cf)
{
    r -> sockfd = fd;
    r -> epfd = epfd;
    r -> pos = r -> last = 0;
    r -> state = 0;
    r -> doc_root = cf -> root;
    INIT_LIST_HEAD(&(r -> list));

    return XM_OK;
}

int xm_free_request(xm_http_request_t *r)
{
    // To Do
    (void)r;
    return XM_OK;
}

int xm_init_out(xm_http_out_t *out, int fd)
{
    out -> fd = fd;
    out -> keep_alive = 0;
    out -> modified = 1;
    out -> status = 0;

    return XM_OK;
}

int xm_free_out(xm_http_out_t *o)
{
    //To Do
    (void)o;
    return XM_OK;
}

void xm_http_handle_header(xm_http_request_t *r, xm_http_out_t *o)
{
    struct list_head *pos;  //指向 list_head 的指针

    xm_http_header_t *hd;
    xm_http_header_handle_t *header_in ;
    int len;

    /* 遍历 r 中list开始的每个节点 ，使 pos 指向它 */
    list_for_each(pos, &r->list) {
        /* 得到与 pos 指向相关的数据结构 就是 xm_http_header_t ,保存在 hd 中*/
        hd = list_entry(pos, xm_http_request_t, list);

        /* 遍历 header_handle_t 结构数组 */
        for(header_in = xm_http_headers_in;
                strlen(header_in->name) > 0;
                header_in++) {
            /* 找到 与 hd 的 名字相同的 */
            if(strncmp(hd->key_start, header_in->name, hd->key_end - hd->key_start) == 0){
                len = hd ->value_end - hd -> value_start;
                /* 调用对应的处理函数 */
                (*(header_in->handler))(r, o, hd->value_start, len);
                break;
            }
        }

        /* 处理完每一个header 就从链表删除 */
        list_del(pos);
        /* 释放 header 结构 */
        free(hd);
    }
}


int xm_http_close_conn(xm_http_request_t *r)
{
    close(r -> sockfd);
    free(r);

    return XM_OK;
}

static int xm_http_process_ignore(xm_http_request_t *r, xm_http_out_t *out, char *data, int len)
{
    (void)r;
    (void)out;
    (void)data;
    (void)len;

    return XM_OK;
}

static int xm_http_process_connection(xm_http_request_t *r, xm_http_out_t *out, char *data, int len)
{
    (void)r;

    /* 判断是否保持连接  忽略大小写 */
    if(strncasecmp("keep-alive", data, len) == 0){
        out -> keep_alive = 1;
    }

    return XM_OK;
}


static int xm_http_process_if_modified_since(xm_http_request_t *r, xm_http_out_t *out, char *data, int len)
{
    (void)r;
    (void)len;

    struct tm tm;
    if(strptime(data, "%a, %d %b %Y %H:%M:%S GMT",&tm) == (char *)NULL){
        return XM_OK;
    }

    time_t client_time = mktime(&tm);

    /* 计算两个时刻之间的时间差 */
    double time_diff = difftime(out -> mtime, client_time);
    if(fabs(time_diff) < 1e-6){
        /* Not Modified */
        out -> modified = 0;
        out -> status = XM_HTTP_NOT_MODIFIED;
    }
    return XM_OK;
}


const char *trans_statuscode_to_shortmsg(int statuscode)
{
    if(statuscode == XM_HTTP_OK){
        return "OK";  
    }else if(statuscode == XM_HTTP_NOT_MODIFIED){
        return "Not Modified";
    }else if(statuscode == XM_HTTP_NOT_FOUND){
        return "Not Found";     
    }else {
        return "Unknown";
    }
}
