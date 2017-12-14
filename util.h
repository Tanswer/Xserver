#ifndef UTIL_H
#define UTIL_H

#include <sys/socket.h>
#include <unistd.h>

#define LISTENMAX 1024

#define Min(x, y) ((x) < (y) ? (x) : (y))

typedef struct xm_conf_s {
    void *root;
    int port;
    int thread_nums;
} xm_conf_t;

int open_listenfd(int port);

int setnonblocking(int fd);

int read_conf(char *file, xm_conf_t *cf, char *buf, int len);

#endif
