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

#include "dbg.h"
#include "epoll.h"
#include "http_request.h"
#include "http.h"
#include "http_parse.h"

/* 服务器根目录 */
static char *doc_root = NULL;

/* 得到文件类型 */
static const char* get_filetype(const char *type);
/* 解析uri 得到文件 */
static void parse_uri(char *uri, int uri_length, char *filename, char *cgiargs);
/* 出错处理 */
static void do_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
/* 静态请求回复 */
static void serve_static(int fd, char *filename, size_t filesize, xm_http_out_t *out);

/* 所以请求文件类型 */
xm_type_t xm_type [] = 
{
    {".html","text/html"},
    {".xml","text/xml"},
    {"xhtml","application/xhtml+xml"},
    {".txt","text/plain"},
    {".jpg","image/jpeg"},
    {NULL,"text/plain"}
};



void do_request(void *ptr)
{
    xm_http_request_t *r = (xm_http_request_t *)ptr;

    int fd = r -> m_sockfd;
    char filename[SHORTLINE];
    struct stat sbuf;
    char *plast = NULL;
    int rc, n;
    size_t remain_size;

    while(1){ /* 循环读取客户数据并分析之 */
        plast = &r -> m_buf[r->m_read_idx % MAX_BUF];
        remain_size = Min(MAX_BUF - (r->m_read_idx - r->m_check_inx) - 1,
                MAX_BUF - r-> m_read_idx % MAX_BUF);
        
        n = read(fd , plast, remain_size);
        check(r->m_read_idx - r->m_check_inx < MAX_BUF, "request buffer overflow!");

        if(n == 0){
            /* 客户端退出 */
            log_info("read return 0,ready to close fd %d,remain_size = %zu",fd,remain_size);
            
            rc = xm_http_close_conn(r);
            check(rc == 0, "do_request:xm_http_close_conn");
        }
        if(n < 0){
            if( errno != EAGAIN ){      /* 出错 */
                log_err("read error, errno  = %d",errno);

                /* 出错关闭 */
                rc = xm_http_close_conn(r);
                check(rc == 0, "do_request:xm_http_close_conn");
            }
            break;  /* EAGAIN */
        }

        r -> m_read_idx += n;
        check(r->m_read_idx - r->m_check_inx < MAX_BUF, "request buffer overflow!");
        
        log_info("ready to  parse request line");
        rc = xm_http_parse_request_line(r);
        if(rc == MM_AGAIN){
            continue;
        }else if(rc != MM_OK){
            log_err("rc != MM_OK");
            
            rc = xm_http_close_conn(r);
            check(rc == 0, "do_request:xm_http_close_conn");
        }

        log_info("method = %.*s",(int)(r->m_method_end - r->m_request_start),(char *)r->m_request_start);
        log_info("uri == %.*s",(int)(r->m_uri_end - r->m_uri_end),(char *)r->m_uri_start);


        rc = xm_http_parse_header(r);
        if(rc == MM_AGAIN){
            continue;
        }else if(rc != MM_OK){
            log_err("rc != MM_OK");
        
            rc = xm_http_close_conn(r);
            check(rc == 0, "do_request:xm_http_close_conn");
        }

        /* 处理HTTP Header */
        xm_http_out_t *out = (xm_http_out_t *)malloc(sizeof(xm_http_out_t));
        if(out == NULL){
            log_err("no enough space for xm_http_out_t");
            exit(1);
        }

        rc = xm_init_out(out, fd);
        check(rc == MM_OK,"xm_init_out");

        parse_uri(r->m_uri_start, r->m_uri_end - r->m_uri_start,filename,NULL);

        if(stat(filename,&sbuf) < 0){
            do_error(fd, filename, "404","Not Found","server can not find the file");
            continue;
        }

        //
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode) ){
            do_error(fd, filename, "403","Forbidden","server can not read the file");
            continue;
        }

        out -> mtime = sbuf.st_mtime;

        xm_http_handle_header(r, out);
        check(list_empty(&(r->list)) == 1, "header list should be empty");

        if(out -> status == 0){
            out -> status = MM_HTTP_OK;
        }

        serve_static(fd, filename, sbuf.st_size, out);

        if(!out -> keep_alive) {
            log_info("no keep_alive! ready to close");
            free(out);
        
            rc = xm_http_close_conn(r);
            check(rc == 0, "do_request:xm_http_close_conn");
        }
        free(out);
    }

    struct epoll_event event;
    event.data.ptr = ptr;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

    xm_epoll_mod(r->m_epfd, r->m_sockfd, &event);
    //xm_add_timer(r, T)
    return ;
}

static void parse_uri(char *uri, int uri_length, char *filename, char *cgiargs)
{
    check(uri !=NULL, "uri is NULL");
    /* uri_length can not be too long */
    if(uri_length > (SHORTLINE >> 1)){
        log_err("uri too long: %.*s",uri_length, uri);
        return ;
    }
    uri[uri_length] = '\0';

    char *question_mark = strchr(uri,'?');
    int filename_length;
    if(question_mark){
        filename_length = (int)(question_mark - uri);
        //
    } else {
        filename_length = uri_length;
        //
    }

    if(cgiargs){
        //to do
    }

    strcpy(filename, doc_root);
   
    strncat(filename,uri,filename_length);

    char *last_comp = strrchr(filename, '/');
    char *last_dot = strrchr(last_comp, '.');
    if(last_dot == NULL && filename[strlen(filename)-1] != '/'){
        strcat(filename,"/");
    }

    if(filename[strlen(filename)-1] == '/'){
        strcat(filename,"index.html");
    }

    log_info("filename = %s",filename);
    return ;
}

static const char *get_filetype(const char* type)
{
    if(type == NULL)
        return "text/plain";

    for(int i=0; xm_type[i].type != NULL; i++){
        if(strcmp(type,xm_type[i].type) == 0)
            return xm_type[i].value;
    }
}
