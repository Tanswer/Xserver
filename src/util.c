/*
 * @filename:    util.c
 * @author:      Tanswer
 * @date:        2017年11月17日 15:09:42
 * @description:
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "util.h"
#include "dbg.h"

int open_listenfd(int port)
{
    int lfd, optval = 1;
    struct sockaddr_in serveraddr;

    if((lfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
        return -1;
    }

    if(setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR,
                (const void *)&optval, sizeof(int)) < 0)
        return -1;
    
    bzero((char *)&serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);

    if(bind(lfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr)) < 0)
        return -1;

    if(listen(lfd, LISTENMAX) < 0)
        return -1;

    return lfd;
}

//设置非阻塞
//成功返回0，失败返回-1
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    if(old_option == -1)
        return -1;
    int new_option = old_option | O_NONBLOCK;
    int res = fcntl(fd, F_SETFL, new_option);
    if(res == -1)
        return -1;
    return 0;
}


int read_conf(char *conf, xm_conf_t *cf, char *buf, int len)
{
    FILE *fp = fopen(conf, "r");
    if (!fp) {
        log_err("cannot open config file: %s", conf);
        return XM_CONF_ERROR;
    }
    char *equalsign_pos;
    int line_length;
    char *cur_pos = buf;

    /* 读取以行 */
    while (fgets(cur_pos, len, fp)) {
        equalsign_pos = strstr(cur_pos, "=");   /* 等于号的位置 */
        line_length = strlen(cur_pos);                                         
        /*
         * debug("read one line from conf: %s, len = %d", cur_pos, line_len);
         */
        if (!equalsign_pos)
            return XM_CONF_ERROR;

        if (cur_pos[strlen(cur_pos) - 1] == '\n') {
            cur_pos[strlen(cur_pos) - 1] = '\0';
        }

        if (strncmp("root", cur_pos, 4) == 0) {
            cf->root = equalsign_pos + 1;
        }
        if (strncmp("port", cur_pos, 4) == 0) {
            cf->port = atoi(equalsign_pos + 1);                                                                      
        }

        if (strncmp("threadnum", cur_pos, 9) == 0) {
            cf->threadnum = atoi(equalsign_pos + 1);                                                             
        }
        
        if (strncmp("queuemaxnum", cur_pos, 11) == 0) {
            cf->queuemaxnum = atoi(equalsign_pos + 1);                                                             
        }
        cur_pos += line_length;
        len -= line_length;
    }
    fclose(fp);
    return XM_CONF_OK;
}
