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
#include <sys/mman.h>
#include <sys/stat.h>

#include "../src/util.h"
#include "../src/dbg.h"
#include "../src/cJSON.h"

int read_conf(char *filename, xm_conf_t *cf)
{

    struct stat sbuf;
    if(stat(filename, &sbuf) < 0){
        log_err("stat error");
    }

    int srcfd = open(filename, O_RDONLY, 0);
    

    char *srcaddr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
    check(srcaddr != (void *)-1, "mmap error");
    close(srcfd);

    //debug("srcaddr = %s",srcaddr);

    cJSON* pJson = cJSON_Parse(srcaddr);
    check(pJson != NULL,"cJSON_Parse");

    cJSON *pSub;

    /* get root */
    pSub = cJSON_GetObjectItem(pJson, "root");
    check(pSub != NULL, "cJSON_GetObjectItem");
    cf -> root = (char *)pSub->valuestring;

    /* get port */
    pSub = cJSON_GetObjectItem(pJson, "port");
    check(pSub != NULL, "cJSON_GetObjectItem");
    cf -> port = pSub->valueint;

    /* get threadnum */
    pSub = cJSON_GetObjectItem(pJson, "threadnum");
    check(pSub != NULL, "cJSON_GetObjectItem");
    cf -> threadnum = pSub->valueint;


    /* get queuemaxnum */
    pSub = cJSON_GetObjectItem(pJson, "queuemaxnum");
    check(pSub != NULL, "cJSON_GetObjectItem");
    cf -> queuemaxnum = pSub->valueint;

    munmap(srcaddr, sbuf.st_size);

    return 0;
}

int main()
{
    xm_conf_t cf;
    char *conf_file = "../xm.conf";

    int rc = read_conf(conf_file, &cf);
    if(rc == 0){
        printf("root === %s\n",cf.root );
        printf("port === %d\n",cf.port );
        printf("threadnum === %d\n",cf.threadnum );
        printf("queuemaxnum === %d\n",cf.queuemaxnum );
    }else{
        printf("read fail\n");
    }

    return 0;
}
