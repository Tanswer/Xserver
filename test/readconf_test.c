/*
 * @filename:    readconf_test.c
 * @author:      Tanswer
 * @date:        2017年12月21日 10:38:53
 * @description:
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../src/util.h"

int read_conf(char *filename, xm_conf_t *cf, char *buf, int len)
{
    FILE *fp;
    fp = fopen(filename, "r");
    if(!fp)
    {
        return -1;
    }

    char *cur_pos = buf;
    int line_length;
    char *equal_pos;

    while(fgets(cur_pos, len, fp)) {
        equal_pos = strstr(cur_pos, "=");
        line_length = strlen(cur_pos);

        if(!equal_pos){
            return -1;
        }

        if(cur_pos[line_length-1] == '\n'){
            cur_pos[line_length-1] = '\0';
        }

        if(strncmp("root", cur_pos, 4) == 0){
            cf -> root = equal_pos+1;
        }
        if(strncmp("port", cur_pos, 4) == 0){
            cf -> port = atoi(equal_pos+1);
        }
        if(strncmp("threadnum", cur_pos, 9) == 0){
            cf -> threadnum = atoi(equal_pos+1);
        }

        cur_pos += line_length;
        len -=line_length;
    }
    fclose(fp);
    return 0;
}

int main()
{
    xm_conf_t cf;
    int buflen = 8192;
    char conf_buf[buflen];
    char *conf_file = "../xm.conf";

    int rc = read_conf(conf_file, &cf, conf_buf, buflen);
    if(rc == 0){
        printf("root = %s\n",cf.root);
        printf("port = %d\n",cf.port);
        printf("threadnum = %d\n",cf.threadnum);
    }else{
        printf("read fail\n");
    }

    return 0;
}
