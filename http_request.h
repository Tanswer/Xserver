#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <time.h>
#include "util.h"

#define MM_OK               0

#define MM_AGAIN            EAGAIN


#define MM_HTTP_OK             200

#define MM_HTTP_NOT_FOUND      404

#define MM_HTTP_NOT_MODIFIED   304

#define MAX_BUF 8124

typedef struct mm_http_request {
    void *m_doc_root;
    int m_sockfd;
    int m_epfd;
    int state;      /* 状态 */

    char m_buf[MAX_BUF];   /* 读缓冲区 */
    /* 标识读缓冲区中已经读入的客户数据的最后一个字节的下一个位置 */
    
    size_t m_check_inx,m_read_idx;
    
    int m_method;     /* method */
    void *m_request_start;
    void *m_method_end;
    void *m_uri_start;
    void *m_uri_end;
    void *m_path_start;
    void *m_path_end;
    void *m_query_start;
    void *m_query_end;
    void *request_end;

} mm_http_request_t;



/* 返回信息 */
typedef struct mm_http_out {
    int fd;
    int keep_alive;
    time_t mtime;       /* 文件修改时间 */  
    int modified;       /* 比较 If-modified-since 字段和 mtime 决定是否值上次修改的文件  */
    
    int status;
}mm_http_out_t;

int mm_init_out(mm_http_out_t *o, int fd);
int mm_free_out(mm_http_out_t *o);

int mm_init_request(mm_http_request_t *r, int fd, int epfd, mm_conf_t *cf);
int mm_free_request(mm_http_request_t *r);

void mm_http_handle_header(mm_http_request_t *r, mm_http_out_t *o);
int mm_http_close_conn(mm_http_request_t *r);

#endif
