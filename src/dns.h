#ifndef DNS_H
#define DNS_H

typedef struct Header
{
    unsigned short TransID;     // 会话标识

    //unsigned short Flags;       // 标志
    unsigned char RD : 1;       // 表示期望递归
    unsigned char TC : 1;       // 表示可截断的
    unsigned char AA : 1;       // 表示可授权回答
    unsigned char opcode : 4;   // 0表示标准查询，1表示反向查询，2表示服务器状态请求
    unsigned char QR : 1;       // 查询/响应位,0为查询，1为响应

    unsigned char rcode : 4;    // 表示返回码
    unsigned char zero : 3;     // 必须为0
    unsigned char RA : 1;       // 表示可用递归

    unsigned short Questions;   // 问题数
    unsigned short AnswerRRs;   // 回答资源记录数
    unsigned short AuthorityRRs;// 授权资源记录数
    unsigned short AdditionRRs; // 附加资源记录数
}DNSHeader;

void Resolve(char *DomainName, char *ips);
int SendDnsPackage(unsigned short id, int *sockfd,const  char *DomainName);
void RecvDnsPackage(unsigned short id, int *sockfd, char *ips);


#endif
