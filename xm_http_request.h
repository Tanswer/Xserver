#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <time.h>
#include "util.h"


/* 处理方法的返回值*/

/* 全局定义的几个错误码 */
#define XM_OK                   0
#define XM_ERROR                -1
#define XM_AGAIN                -2

/* 响应状态码 */
#define XM_HTTP_OK             200
#define XM_HTTP_NOT_MODIFIED   304
#define XM_HTTP_NOT_FOUND      404


#define MAX_BUF 8124

/* 请求的所有信息 结构 */
typedef struct xm_http_request_s {
    void *m_doc_root;
    int m_sockfd;
    int m_epfd;
    int state;              /* 状态 */

    char m_buf[MAX_BUF];    /* 读缓冲区 */
    /* 标识读缓冲区中已经读入的客户数据的最后一个字节的下一个位置 */
    
    size_t m_check_inx,m_read_idx;
    
    int m_method;           /* method */
    void *m_request_start;
    void *m_method_end;
    void *m_uri_start;
    void *m_uri_end;
    void *m_path_start;
    void *m_path_end;
    void *m_query_start;
    void *m_query_end;
    void *request_end;

} xm_http_request_t;



/* 响应消息结构体 */
typedef struct xm_http_out_s {
    int fd;
    int keep_alive;
    time_t mtime;       /* 文件修改时间 */  
    int modified;       /* 比较 If-modified-since 字段和 mtime 决定是否值上次修改的文件  */
    
    int status;
}xm_http_out_t;

int xm_init_out(xm_http_out_t *o, int fd);
int xm_free_out(xm_http_out_t *o);

int xm_init_request(xm_http_request_t *r, int fd, int epfd, xm_conf_t *cf);
int xm_free_request(xm_http_request_t *r);

void xm_http_handle_header(xm_http_request_t *r, xm_http_out_t *o);
int xm_http_close_conn(xm_http_request_t *r);

#endif
