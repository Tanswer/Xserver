#ifndef DBG_H
#define DBG_H

/* 
 * 一些可变参数宏的定义 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


/* 在调试时输出调试信息 */
//#ifdef DEBUG
//#define debug(M, ...) fprintf(stderr,"DEBUG %s : %d: " M "\n",__FILE__,__LINE__, ##__VA_ARGS__)       /*  */
//#else
//#define debug(M, ...)       // 空宏 
//#endif

#define debug(M, ...) fprintf(stderr,"DEBUG %s : %d: " M "\n",__FILE__,__LINE__, ##__VA_ARGS__)       /*  */

/* 翻译错误代码值 */
#define trans_errno() (errno == 0 ? "None" : strerror(errno))

/* 记录日志 */
#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno:%s) " M "\n", __FILE__, __LINE__, trans_errno(), ##__VA_ARGS__)
#define log_warn(M, ...) fprintf(stderr, "[WARN] (%s:%d: errno:%s) " M "\n", __FILE__, __LINE__, trans_errno(), ##__VA_ARGS__)


/* 通用判断返回值函数 */
#define check(A, M, ...) if(!(A)) {log_err(M "\n", ##__VA_ARGS__); }
#define check_exit(A, M, ...) if(!(A)) {log_err(M "\n", ##__VA_ARGS__); exit(1); }

#endif
