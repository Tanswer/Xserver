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

    int fd = r -> sockfd;
    char filename[SHORTLINE];
    struct stat sbuf;
    char *plast = NULL;
    int rc, n;
    size_t remain_size;

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
        
        log_info("ready to  parse request line");
        rc = xm_http_parse_request_line(r);
        if(rc == XM_AGAIN){
            continue;
        }else if(rc != XM_OK){
            log_err("rc != XM_OK");
           
            goto err;
        }

        
        printf("method = %.*s",(int)(r->method_end - r->request_start + 1),(char *)r->request_start);
        printf("uri == %.*s",(int)(r->uri_end - r->uri_end),(char *)r->uri_start);
        
        log_info("method = %.*s",(int)(r->method_end - r->request_start + 1),(char *)r->request_start);
        log_info("uri == %.*s",(int)(r->uri_end - r->uri_end),(char *)r->uri_start);


        rc = xm_http_parse_header_line(r);
        if(rc == XM_AGAIN){
            continue;
        }else if(rc != XM_OK){
            log_err("rc != XM_OK");
       
            goto err;
        }

        /* 处理HTTP Header */
        xm_http_out_t *out = (xm_http_out_t *)malloc(sizeof(xm_http_out_t));
        if(out == NULL){
            log_err("no enough space for xm_http_out_t");
            exit(1);
        }

        rc = xm_init_out(out, fd);
        check(rc == XM_OK,"xm_init_out");

        parse_uri(r->uri_start, r->uri_end - r->uri_start,filename,NULL);
        printf("filename = %s\n",filename);

        /* 404 */
        if(stat(filename,&sbuf) < 0){
            do_error(fd, filename, "404","Not Found","server can not find the file");
            continue;
        }

        /* 403 */
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode) ){
            do_error(fd, filename, "403","Forbidden","server can not read the file");
            continue;
        }

        /* 最后修改时间 */
        out -> mtime = sbuf.st_mtime;

        xm_http_handle_header(r, out);
        check(list_empty(&(r->list)) == 1, "header list should be empty");

        if(out -> status == 0){
            out -> status = XM_HTTP_OK;
        }

        serve_static(fd, filename, sbuf.st_size, out);

        if(!out -> keep_alive) {
            log_info("no keep_alive! ready to close");
            free(out);
        
            goto close;
        }
        free(out);
    } 

    struct epoll_event event;
    event.data.ptr = ptr;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

    xm_epoll_mod(r->epfd, r->sockfd, &event);
    //xm_add_timer(r, T)
    return ;

err:
close:
    rc = xm_http_close_conn(r);
    check(rc == 0, "do_request:xm_http_close_conn");

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

    int i;
    for(i=0; xm_type[i].type != NULL; i++){
        if(strcmp(type,xm_type[i].type) == 0)
            return xm_type[i].value;
    }
    return xm_type[i].value;
}

static void do_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char header[MAXLINE], body[MAXLINE];

    sprintf(body, "<html><title>Xmserver Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\n", body);
    sprintf(body, "%s%s:%s\n",body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\n</p>",body, longmsg, cause);
    sprintf(body, "%s<hr><em>XM Web Server</em>\n</body></html>",body);

    sprintf(header, "HTTP/1.1 %s %s\r\n",errnum, shortmsg);
    sprintf(header, "%sServer:Xmserver\r\n",header);
    sprintf(header, "%sContent-type: text/html\r\n",header);
    sprintf(header, "%sConnection: close\r\n",header);
    sprintf(header, "%sContent-lenght: %d\r\n\r\n",header, (int)strlen(body));

    rio_writen(fd, header, strlen(header));
    rio_writen(fd, body, strlen(body));

    return ;
}

static void serve_static(int fd, char *filename, size_t filesize, xm_http_out_t *out)
{
    char header[MAXLINE];
    char buf[SHORTLINE];
    size_t n;
    struct tm tm;
    /* 得出文件类型 */
    const char *dot_pos = strrchr(filename, '.');
    const char *filetype = get_filetype(dot_pos);

    sprintf(header, "HTTP/1.1 %d %s\r\n",out -> status, trans_statuscode_to_shortmsg(out->status));

    if(out -> keep_alive) {
        sprintf(header, "%sConnection: keep-alive\r\n",header);
        sprintf(header, "%sKeep-Alive: timeout=%d\r\n",header,TIMEOUT_DEFAULT);
    }


    if(out -> modified){
        sprintf(header, "%sContent-type: %s\r\n",header, filetype);
        sprintf(header, "%sContent-lenght: %zu\r\n",header, filesize);
        
        /*把系统时间转换为本地时间  localtime_r 是线程安全的,详细请 man localtime_r*/
        localtime_r(&(out->mtime), &tm);
        /* format date and time */
        strftime(buf, SHORTLINE, "%a, %d %b %Y %H:%M:%S GMT",&tm);

        sprintf(header, "%sLast-Modified: %s\r\n",header, buf);
    }

    sprintf(header, "%sServer: Xmserver\r\n",header);
    sprintf(header, "%s\r\n",header);
    /* header end */
    
    n = (size_t)rio_writen(fd, header, strlen(header));    
    check(n == strlen(header), "rio_writen error, errno = %d",errno);

    if(!out -> modified){
        return ;
    }

    int srcfd = open(filename, O_RDONLY, 0);
    check(srcfd > 2, "open error");

    char *srcaddr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    check(srcaddr != (void *)-1, "mmap error");
    close(srcfd);

    n = rio_writen(fd, srcaddr, filesize);
    check(n == filesize, "rio_written error");

    munmap(srcaddr, filesize);

}
