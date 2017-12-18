#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <time.h>
#include "list.h"
#include "util.h"



#define XM_HTTP_PARSE_INVALID_METHOD    10
#define XM_HTTP_PARSE_INVALID_REQUEST   11
#define XM_HTTP_PARSE_INVALID_HEADER    12

#define XM_HTTP_UNKNOWN     0x0001
#define XM_HTTP_GET         0x0002
#define XM_HTTP_HEAD        0x0004
#define XM_HTTP_POST        0x0008
#define XM_HTTP_PUT         0x0010

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
    void *doc_root;
    int sockfd;
    int epfd;
    int state;              /* 解析状态 */

    char buf[MAX_BUF];      /* 读缓冲区 */
    /* 标识读缓冲区中已经读入的客户数据的最后一个字节的下一个位置 */
    
    size_t pos, last;       /* 解析的位置和read 读取的位置*/
    
    int method;             /* method */
    void *method_end;       /* GET 等字符串结尾*/
    
    void *uri_start;        /*指向 uri的开始*/
    void *uri_end;          /*指向 uri 结束后的下一个地址*/
    
    /*HTTP/1.1 前面的1代表major，后面的1代表 minor*/
    int http_major;
    int http_minor;

    /*通过request_start和end 获取完整请求行*/
    void *request_start;    /*请求行开始处*/
    void *request_end;      /*请求行结尾处*/

    void *path_start;
    void *path_end;
    void *query_start;
    void *query_end;

    struct list_head list;  /* 保存 header */
    void *cur_header_key_start;
    void *cur_header_key_end;
    void *cur_header_value_start;
    void *cur_header_value_end;

    void *timer;

} xm_http_request_t;



/* 响应消息结构体 */
typedef struct xm_http_out_s {
    int fd;
    int keep_alive;
    time_t mtime;       /* 文件修改时间 */  
    int modified;       /* 比较 If-modified-since 字段和 mtime 决定是否值上次修改的文件  */
    
    int status;
}xm_http_out_t;


typedef struct xm_http_header_s {
    void *key_start, *key_end;
    void *value_start, *value_end;
    list_head list;
}xm_http_header_t;

/* data 为 key 对应的 value */
typedef int (*xm_http_header_handler_pt) (xm_http_request_t *r, xm_http_out_t *o, char *data, int len);

typedef struct {
    char *name;                         //header 中的 key
    xm_http_header_handler_pt handler;  // key 对应的处理函数
}xm_http_header_handle_t;


int xm_init_out(xm_http_out_t *o, int fd);
int xm_free_out(xm_http_out_t *o);

int xm_init_request(xm_http_request_t *r, int fd, int epfd, xm_conf_t *cf);
int xm_free_request(xm_http_request_t *r);

void xm_http_handle_header(xm_http_request_t *r, xm_http_out_t *o);
int xm_http_close_conn(xm_http_request_t *r);

const char *trans_statuscode_to_shortmsg(int statuscode);

#endif
