#ifndef HTTP_H
#define HTTP_H

#include "xm_http_request.h"

#define MAXLINE     8192
#define SHORTLINE   512

#define CR '\r'
#define LF '\n'
#define CRLFCRLF "\r\n\r\n"

typedef struct xm_type_s {
    const char *type;
    const char *value;
}xm_type_t;

void do_request(void *);

int xm_http_parse_request_line(xm_http_request_t *r);
int xm_http_parse_header_line(xm_http_request_t *r);

#endif
