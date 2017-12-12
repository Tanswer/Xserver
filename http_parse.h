#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#include "http_request.h"

int mm_http_parse_request_line(mm_http_request_t *r);
int mm_http_parse_header(mm_http_request_t *r);


#endif
