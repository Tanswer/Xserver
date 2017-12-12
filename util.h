#ifndef UTIL_H
#define UTIL_H

#include <sys/socket.h>
#include <unistd.h>

#define LISTENMAX 1024

#define Min(x, y) ((x) < (y) ? (x) : (y))

typedef struct mm_conf {
    void *root;
    int port;
    int thread_nums;
} mm_conf_t;

int open_listenfd(int port);

int setnonblocking(int fd);

int read_conf(char *file, mm_conf_t *cf, char *buf, int len);

#endif
