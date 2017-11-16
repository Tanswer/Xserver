/*
 * @filename:    httpd.c
 * @author:      Tanswer
 * @date:        2017年11月13日 17:09:20
 * @description:
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>


#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server:jdbhttpd/0.1.0\r\n"
typedef unsigned short u_short;

void accept_request(int );      //接收请求
void bad_request(int );         // 400
void cat(int , FILE*);          //返回文件内容
void cannot_execute(int );      //cgi不能执行  500
void error_die(const char *);           //错误处理
void execute_cgi(int ,const char *, const char *, const char *);    //执行对应cgi脚本
int get_line(int ,char*, int);  //接收 client 一行内容
void headers(int , const char *);   //200 OK 返回文件头
void not_found(int );           //not found 404
void serve_file(int ,const char *);     //处理静态文件 发送headers和body
int startup(u_short *);         //创建一个listenfd
void unimplemented(int );       //不是GET||POST 未实现 501

void accept_request(int client)
{
    char buf[1024];
    int numchars;
    char method[255];
    char url[255];
    char path[512];

    size_t i,j;
    struct stat st;
    int cgi = 0;    //become true if is cgi

    char *query_string = NULL;

    //读http请求的第一行数据,放到buf中
    numchars = get_line(client, buf, sizeof(buf));
    i = 0;
    j = 0;
    while(!ISspace(buf[j]) && (i<sizeof(method)-1))
    {
        method[i] = buf[j];
        i++;
        j++;
    }
    method[i] = '\0';

    //如果不是get或者post则返回response告诉客户端没实现该方法
    if(strcasecmp(method,"GET") && strcasecmp(method,"POST"))
    {
        unimplemented(client);
        return ;
    }

    //如果是POST方法就将cgi标志置为1
    if(strcasecmp(method,"POST") == 0)
    {
        cgi = 1;
    }
     
    i = 0;
    //跳过所有空格
    while(ISspace(buf[j]) && (j<sizeof(buf)))
        j++;
    
    //解析出url
    while(!ISspace(buf[j]) && (i<sizeof(url)-1) && (j<sizeof(buf)))
    {
        url[i] = buf[j];
        i++;
        j++;
    }
    url[i] = '\0';

    //如果是GET方法
    if(strcasecmp(method,"GET") == 0)
    {
        //用一个指针去指向url
        query_string = url;
        //去遍历url，找到?  如果没找到则退出循环
        while((*query_string != '?') && (*query_string != '\0'))
            query_string++;

        //退出循环后，检查是碰到?还是到了url的结尾
        if(*query_string == '?')
        {
            //如果是?的话，说明是动态获取，有参数
            cgi = 1;
            //从?开始把url分为两部分
            *query_string = '\0';
            query_string++;
        }
    }

    sprintf(path,"htdocs%s",url);
    
    if(path[strlen(path)-1] == '/')
        strcat(path,"index.html");

    //在系统上查询文件是否存在
    if(stat(path,&st) == -1)
    {
        //如果不存在,那把这次http的请求后续内容全部读完并忽略
        //(忽略为什么还要写)
        while((numchars > 0) && strcmp(buf, "\n"))
            numchars = get_line(client, buf, sizeof(buf));
        //返回 404 给客户端
        not_found(client);
    }
    else {
        //此时文件存在，与S_IFMT相与
        if((st.st_mode & S_IFMT) == S_IFDIR)
            //如果是目录，那就加上index.html
            strcat(path,"/index.html");

        if((st.st_mode & S_IXUSR) || 
                (st.st_mode & S_IXGRP) ||
                (st.st_mode & S_IXOTH) )
            //如果是可执行文件，不论是属于用户/组/其他这三者类型，就将cgi置为1
            cgi = 1;
        if(!cgi)
            serve_file(client,path);
        else
            execute_cgi(client, path,method, query_string);
    }
    close(client);
}

void bad_request(int client)
{
    char buf[1024];
    sprintf(buf,"HTTP/1.0 400 BAD REQUEST\r\n");
    send(client,buf,sizeof(buf),0);
    sprintf(buf,"Content-type:text/html\r\n");
    send(client,buf,sizeof(buf),0);
    sprintf(buf,"\r\n");
    send(client,buf,sizeof(buf),0);
    sprintf(buf,"<p>Your brower sent a bad request, ");
    send(client,buf,sizeof(buf),0);
    sprintf(buf,"such as a POST without a Content-Length.\r\n");
    send(client,buf,sizeof(buf),0);
}


void cat(int client, FILE* resource)
{
    char buf[1024];

    //从文件描述符中读取指定内容
    fgets(buf, sizeof(buf), resource);

    while(!feof(resource))
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}


void cannot_execute(int client)
{
    char buf[1024];
     sprintf(buf,"HTTP/1.0 500 Internet Server Error\r\n");
     send(client, buf, sizeof(buf), 0);
     sprintf(buf, "Content-type:text/html\r\n");
     send(client, buf, sizeof(buf), 0);
     sprintf(buf, "\r\n");
     send(client, buf, sizeof(buf), 0);
     sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
     send(client, buf, sizeof(buf), 0);
}

void error_die(const char *sc)
{
    perror(sc);
    exit(1);
}


void execute_cgi(int client, const char *path, const char *method, const char *query_string)
{
    char buf[1024];
    int cgi_outpu[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;  
    int content_length = 1;

    //往buf中添加内容以保证进入下面的while
    buf[0] = 'A';   buf[1] = '\0';


    //如果是http 请求是GET方法的话 读取并忽略后面的请求
    if(strcasecmp(method,"GET") == 0)
        while((numchars > 0) && strcmp("\n",buf))
            numchars = get_line(client, buf, sizeof(buf));
    else{ //POST
        
    }
    
}


int get_line(int sock, char *buf, int size)
{
    int i = 0;
    int n;
    char c = '\0';

    while((i < size-1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);

        if(n > 0)
        {
            if(c == '\r')
            {
                n = recv(sock, &c ,1, MSG_PEEK);

                if((n>0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';
    return i;
}


void headers(int client, const char * filename)
{
    char buf[1024];
    //use filename to determine file type
    (void)filename;

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);

    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);

    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);

    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}


void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client ,buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);


    sprintf(buf, "<HTML><TITLE>404 NOT FOUND</TITLE>\r\n");
    send(client , buf,strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);    
    sprintf(buf, "your request because the resouce specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);

}


void serve_file(int client , const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    //确保buf里面有东西，能进入下面的循环
    buf[0] = 'A';    buf[1] = '\0';
    
    //循环的作用是读取并忽略点这个http后面的所有内容
    while((numchars > 0) && strcmp("\n",buf))
        numchars = get_line(client,buf, sizeof(buf));

    //打开这个路径传进来的文件
    resource = fopen(filename,"r");
    if(resource == NULL)
        not_found(client);

    else
    {
        //打开成功后将这个文件基本信息封装成response的header并返回
        headers(client, filename);
        //接着将这个文件的内容读出来并作为response的body返回
        cat(client, resource);
    }

    fclose(resource);
}




int startup(u_short *port)
{
    int httpd =0;
    struct sockaddr_in name;
    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if(httpd == -1)
        error_die("socket");
    
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        error_die("bind");

    /*如果调用bind后端口号仍然是0,则手动调用getsockname获取端口号*/
    if(*port == 0)
    {
        int namelen = sizeof(name);
        if(getsocketname(httpd,(struct sockaddr *)&name, &namelen) == -1)
            error_die("getsocketname");
        *port = ntohs(name.sin_port);
    }

    if(listen(httpd,5) < 0)
        error_die("listen");

    return (httpd);
}

void unimplemented(int client)
{
    char buf[1024];
    
    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}


int main(int argc,char **argv)
{
    int server_sock = -1;
    u_short port = 0;
    int client_sock = -1;

    struct sockaddr_in client_name;
    int client_name_len = sizeof(client_name);

    server_sock = startup(&port);
    printf("httpd running on port %d\n",port);

    while(1)
    {
        //阻塞等待客户端连接
        client_sock = accept(server_sock, 
                (struct socket_addr *)&client_name,
                &client_name_len);
        if(client_sock == -1)
            error_die("accept");

        accept_request(client_sock);

    }
    return 0;
}






