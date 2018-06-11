/*
 * @filename:    xm_main.c
 * @author:      Tanswer
 * @date:        2017年12月20日 11:28:44
 * @description:
 */

#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "xm_http.h"
#include "xm_http_request.h"
#include "dbg.h"
#include "threadpool.h"
#include "util.h"
#include "timer.h"
#include "epoll.h"
//#include "daemon.h"

#define CONF "/Home/Tanswer/Code/NP/http/xm.conf"

extern struct epoll_event *events;


/* 需要分析如何解析命令行参数 */
static const struct option long_options[]=
{
    {"help", no_argument,NULL,'?'},
    {"conf",required_argument,NULL,'c'},
    {NULL,0,NULL,0}
};

static void usage() {
    fprintf(stderr,
            "Xserver [option]... \n"
            "  -c|--conf <config file>  Specify config file. Default ./xm.conf.\n"
            "  -?|-h|--help             This information.\n"
            );
}


int main(int argc, char *argv[])
{
    int rc;     //保存返回结果
    char *conf = CONF;
    

    /*
     * parse argv 
     * more detail visit: http://www.gnu.org/software/libc/manual/html_node/Getopt.html
     */
    int opt = 0;
    int options_index = 0;

    if (argc == 1) {
        usage();
        return 0;                    
    }

    while ((opt=getopt_long(argc, argv,"c:?h",long_options,&options_index)) != -1) {
        switch (opt) {
            case  0 : 
                break;
            case 'c':
                conf = optarg;
                break;
            case ':':
            case 'h':
            case '?':
                usage();
                return 0;
        }
    }
    debug("conffile = %s", conf);
    if (optind < argc) {
        log_err("non-option ARGV-elements: ");
        while (optind < argc)
            log_err("%s ", argv[optind++]);
        return 0;
    }




    /*
     * read confile file
     */
    xm_conf_t cf;
    rc = read_conf(conf, &cf);
    check(rc == XM_CONF_OK, "read_conf");


    /*
    * install signal handle for SIGPIPE
    * 默认情况下往一个关闭的 socket 中写数据将引发 SIGPIPE 信号，
    * 程序接收到 SIGPIPE 信号将结束进程(这就是为什么服务器有时候莫名退出)
    * 所以需要在代码中捕获并处理该信号
    */
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;    // SIG_IGN 忽略此信号
    sa.sa_flags = 0; 
    if (sigaction(SIGPIPE, &sa, NULL)) {
        log_err("install sigal handler for SIGPIPE failed");
        return 0;

    }

//    Init_daemon();
    
    /* start listen */
    int lfd;
    struct sockaddr_in clientaddr;
    memset(&clientaddr, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(clientaddr);

    lfd = open_listenfd(cf.port);
    rc = setnonblocking(lfd);
    check(rc == 0,"setnonblocking");

    int epfd = xm_epoll_create(0);
    struct epoll_event event;

    xm_http_request_t *request = (xm_http_request_t *)malloc(sizeof(xm_http_request_t));
    xm_init_request(request, lfd, epfd, &cf);

    event.data.ptr = (void *)request;
    event.events = EPOLLIN | EPOLLET;
    xm_epoll_add(epfd, lfd, &event);

    /*
     * create threadpool
     */
    xm_threadpool_t *threadpool = threadpool_init(cf.threadnum, cf.queuemaxnum);
    check(threadpool != NULL, "threadpool_init error");


    /*
     * init timer
     */
    xm_timer_init();

    log_info("Xserver start");
    int i, n, fd;
    int time;

    while(1){
        
        time = xm_find_timer();
        debug("epoll wait time = %d",time);

        n = xm_epoll_wait(epfd, events, MAXEVENTS, time);
        
        xm_handle_expire_timers();

        for(i = 0; i<n; i++){
            xm_http_request_t *r = (xm_http_request_t *)events[i].data.ptr;
            fd = r -> sockfd;

            if(fd == lfd){
                /* yet have one or more connections */
                int connfd;
                while(1){
                    connfd = accept(lfd, (struct sockaddr *)&clientaddr, &len);
                    
                    if(connfd < 0){
                        if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
                            break;      //done
                        }else{
                            log_err("accept");
                            break;
                        }
                    }

                    /*
                     * 得到对方地址
                     */
                    struct sockaddr_in s;
                    socklen_t len = sizeof(s);
                    if(!getpeername(connfd, (struct sockaddr *)&s, &len)){
                        debug("peer ip : %s",inet_ntoa(s.sin_addr));
                        debug("peer port : %d",ntohs(s.sin_port));
                    }


                    rc = setnonblocking(connfd);
                    check(rc == 0, "setnonblocking");
                    log_info("new connection fd %d",connfd);

                    xm_http_request_t *request = (xm_http_request_t *)malloc(sizeof(xm_http_request_t));
                    if(request == NULL){
                        log_err("malloc(sizeof(xm_http_request_t))");
                        break;
                    }

                    xm_init_request(request, connfd, epfd, &cf);
                    event.data.ptr = (void *)request;
                    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

                    xm_epoll_add(epfd, connfd, &event);

                    /* 添加超时事件，时间为默认的，如果超时就关闭连接 */
                    xm_add_timer(request, TIMEOUT_DEFAULT, xm_http_close_conn);

                } //end of while of accept 
            } else {
                if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN))) {
                    log_err("epoll error fd: %d", fd);
                    close(fd);
                    continue;
                }

                log_info("new data from fd %d",fd);
                rc = threadpool_add_task(threadpool, do_request, events[i].data.ptr, 0);
                if(rc == 0){
                    int tag = threadpool_resize(threadpool, cf.threadnum*2, cf.queuemaxnum*2);
                    check(tag == 0, "threadpool_resize error");
                }
//                do_request(events[i].data.ptr);
            }
        }
    }

    return 0;
}
    
