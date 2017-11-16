#ifndef DBG_H
#define DBG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno:%s) " M "\n".__FILE__,__LINE__,clean_errno(), ##__VA_ARGS__)

#define check(A, M, ...) if(!(A)) {log_err(M "\n", ##__VA__ARGS__); }

#endif
