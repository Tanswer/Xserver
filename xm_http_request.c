/*
 * @filename:    xm_http_request.c
 * @author:      Tanswer
 * @date:        2017年12月13日 17:47:58
 * @description:
 */

#include <time.h>
#include "xm_http_request.h"

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
    return XM_OK;
}

int xm_init_out(xm_http_out_t *out, fd)
{
    out -> fd = fd;
    out -> keep_alive = 0;
    out -> modified = 1;
    out -> status = 0;

    return XM_OK;
}

int xm_free_out(xm_http_out_t *out)
{
    //To Do
    return XM_OK;
}

void xm_http_handle_header(xm_http_request_t *r, xm_http_out_t *o)
{
    list_head *pos;

    xm_http_header_t *hd;
    xm_http_header_handle_t *header_in ;
    int len;

    list_for_each(pos, &(r->list)) {
        hd = list_entry(pos, xm_http_header_t, list);

        for(header_in = xm_http_headers_in;
                strlen(header_in->name) > 0;
                header_in++) {
            if(strncmp(hd->key_start, header_in->name, hd->key_end - hd->key_start) == 0){
                len = hd ->value_end - hd -> value_start;
                (*(header_in->handler))(r, o, hd->value_start, len);
                break;
            }
        }

        list_del(pos);
        free(hd);
    }
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
