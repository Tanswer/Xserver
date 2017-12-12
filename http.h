#ifndef HTTP_H
#define HTTP_H

#define MAXLINE     8192
#define SHORTLINE   512



typedef struct mm_type {
    const char *type;
    const char *value;
}mm_type_t;


void do_request(void *);

#endif
