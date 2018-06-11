/*
 * @filename:    dns.c
 * @author:      Tanswer
 * @date:        2018年06月11日 10:14:04
 * @description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#include "dns.h"
#include "dbg.h"

int SendDnsPackage(unsigned short id,
                    int *sockfd,
                    const char *DomainName)
{
    int ret = -1;
    unsigned int DNLength = strlen(DomainName);
    if('.' == DomainName[0] || DNLength < 1 || DNLength > 63)
        return ret;

    /* 将域名转换为符合查询报文的格式 */
    unsigned int QueryNameLength = 0;
    // 查询名  不使用填充字节，长度不固定
    char *QueryName = (char*)malloc(DNLength + 1 + 1);
    if(QueryName == NULL)
    {
        perror("malloc");
        goto exit;
    }
    memset(QueryName, 0, DNLength + 2);

    // 以 . 结尾的 DomainName
    unsigned int pos = 0, i;
    for(i=0; i<DNLength; i++){
        if(DomainName[i] == '.'){
            QueryName[pos] = i - pos;
            if(QueryName[pos] > 0)
                memcpy(QueryName + pos + 1, DomainName + pos, i - pos);
            pos = i + 1;
        }
    }

    // 不以 . 结尾，继续转换
    if(DomainName[i - 1] != '.')
    {
        QueryName[pos] = i - pos;
        memcpy(QueryName + pos + 1, DomainName + pos, i - pos);
        QueryNameLength = DNLength + 2;
    }
    else
    {
        QueryNameLength = DNLength + 1;
    }


    // 填充内容 header + name + type + class
    DNSHeader  *DNSPackage = (DNSHeader*)malloc(sizeof(DNSHeader) + QueryNameLength + 4);
    if(DNSPackage == NULL)
    {
        perror("DNSPackage malloc");
        goto exit;
    }
    
    memset(DNSPackage, 0, sizeof(DNSHeader)+ QueryNameLength + 4);

    // 填充头部
    DNSPackage -> TransID = htons(id);
    DNSPackage -> RD = 0x1; 
    DNSPackage -> Questions = htons(0x1);

    // 填充正文
    char *pText = (char*)DNSPackage + sizeof(DNSHeader);
    memcpy(pText, QueryName, QueryNameLength);

    unsigned short *QueryType = (unsigned short*)(pText + QueryNameLength);
    *QueryType = htons(0x1);  // TYPE A

    ++QueryType;
    *QueryType = htons(0x1);  // CLASS IN

    // 构造一个客户端发送查询报文
    socklen_t addrlen;
    struct sockaddr_in addr;

    addrlen = sizeof(struct sockaddr_in);
    bzero(&addr, addrlen);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    addr.sin_addr.s_addr = inet_addr("8.8.4.4");

    // 发送
    debug("SendDnsPackage sockfd = %d", *sockfd);
    sendto(*sockfd, (char*)DNSPackage, sizeof(DNSHeader)+QueryNameLength + 4, 0,
            (struct sockaddr*)&addr, addrlen);
   
    ret = 0;    

exit:
    if(QueryName)
    {
        free(QueryName);
        QueryName = NULL;
    }
    if(DNSPackage)
    {
        free(DNSPackage);
        DNSPackage = NULL;
    }
    return ret;
}

void RecvDnsPackage(unsigned short id,
                    int *sockfd,
                    char *ips)
{
    socklen_t addrlen;
    struct sockaddr_in addr;
    char buffer[256] = {};

    addrlen = sizeof(struct sockaddr_in);
    bzero(&addr, addrlen);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    addr.sin_addr.s_addr = inet_addr("8.8.8.8");

    int ret = recvfrom(*sockfd,buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addrlen );
    debug("recvfrom ret = %d", ret);

    check(ret > 0, "recvfrom"); 
    
    // 解析收到的内容
    DNSHeader *DNSPackage = (DNSHeader*)buffer;
    unsigned int total = ret;   // 总字节数 包含header
    unsigned int totalText = ret;
    if(totalText <= sizeof(DNSHeader))
    {
        printf("接收到的长度不合法\n");
        return ;
    }

    if(htons(id) != DNSPackage -> TransID)
    {
        printf("接收到的报文id与查询报文不相符\n");
        return ;
    }
    if(0x01 != DNSPackage -> QR)
    {
        printf("接收到的报文不是响应报文\n");
        return ;
    }


    unsigned char *pQueries = (unsigned char*)DNSPackage + sizeof(DNSHeader);
    totalText -= sizeof(DNSHeader);

    // 跳过查询名  查询名最后要求是0
    for(; *pQueries && totalText > 0; ++pQueries,--totalText){;}
    ++pQueries;
    --totalText;

    unsigned short QueryType = ntohs(*((unsigned short*)pQueries));
    pQueries += 2;
    totalText -= 2;

    unsigned short QueryClass = ntohs(*((unsigned short*)pQueries));
    pQueries += 2;
    totalText -= 2;

    // 解析 Answers 字段
    unsigned char *pAnswers = pQueries;
    while(totalText > 0 && totalText <= total)
    {
        // 跳过 域名字段(2字节或不定长)
        if(*pAnswers == 0xC0)   // 存放的是指针
        {
            if( totalText < 2 )
            {
                printf("接收到的内容长度不合法\n");
                return ;
            }
            pAnswers += 2;  // 跳过两个字节的指针
            totalText -= 2;
        }
        else    // 存放的是域名
        {
            for(; *pAnswers && totalText > 0; ++pAnswers, --totalText){;}
            pAnswers++;
            totalText--;
        }
        if(totalText < 4)
        {
            printf("接收到的内容长度不合法");
            return ;
        }

        unsigned short AnswerType = ntohs(*( (unsigned short*)pAnswers) );
        pAnswers += 2;
        totalText -= 2;

        unsigned short AnswerClass = ntohs(*( (unsigned short*)pAnswers) );
        pAnswers += 2;
        totalText -= 2;
    
        if(AnswerType != QueryType || AnswerClass != QueryClass)
        {
//            printf("接收到的内容Type 或者 Class 与查询报文不一致\n");
            return ;
        }

        // 跳过 生存时间字段
        pAnswers += 4;
        totalText -= 4;

        if(htons(0x04) != *(unsigned short*)pAnswers)
        {
            // 跳过 Data length 字段
            totalText -= 2;
            // 跳过真正的 
            totalText -= ntohs(*(unsigned short*)pAnswers);
            
            pAnswers += 2;
            pAnswers += ntohs(*(unsigned short*)pAnswers);
        }
        else
        {
            if(totalText < 6)
            {
                printf("接收到的内容长度不合法\n");
                return ;
            }
            totalText -= 6;
            
            if(AnswerType == 1 && AnswerClass == 1)
            {
                pAnswers += 2;

                unsigned int ip = *(unsigned int*)pAnswers;
                struct in_addr in;  // 用来表示一个32位的IPv4地址
                in.s_addr = ip;
                //printf("IP:%s\n", inet_ntoa(in));
                
                strcat(ips, inet_ntoa(in));
                strcat(ips, ";");
                // tmd 错了这一步调了一个多小时
                pAnswers += 4;
            }
            else
            {
                pAnswers += 6;
            }
        }
    }
}

void Resolve(char *DomainName, char *ips)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    check(sockfd >= 0, "socket error");

    int timeout = 2000;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(int));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));

    srand((unsigned int)time(NULL));
    unsigned short id = (unsigned short)rand();

    SendDnsPackage(id, &sockfd, DomainName);

    RecvDnsPackage(id, &sockfd, ips);

    close(sockfd);
}



