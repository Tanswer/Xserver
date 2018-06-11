/*
 * @filename:    dns_test.c
 * @author:      Tanswer
 * @date:        2018年06月11日 17:50:07
 * @description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/dns.h"

int main(void)
{
    char domainname[64];
    printf("请输入DomainName: ");
    scanf("%s", domainname);

    Resolve(domainname);
    return 0;
}

