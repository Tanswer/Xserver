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
#include <netinet/in.h>
#include "xm_http.h"
#include "xm_http_request.h"
#include "dbg.h"
//#include "threadpool.h"
#include "util.h"
#include "timer.h"
#include "epoll.h"

#define CONF "xm.conf"

extern struct epoll_event *events;

static const struct option long_options[]=
{
    {"help", no_argument,NULL,'?'},
    {"version", no_argument,NULL,'V'},
    {"conf",required_argument,NULL,'c'},
    {NULL,0,NULL,0}
};

static void usage() {
    fprintf(stderr,
            "zaver [option]... \n"
            "  -c|--conf <config file>  Specify config file. Default ./xm.conf.\n"
            "  -?|-h|--help             This information.\n"
            "  -V|--version             Display program version.\n"
            );
}


int main(int argc, char *argv[])
{
    int rc;     //保存返回结果
    int lfd;
    char *conf = CONF;
    int opt = 0;
    int options_index = 0;
    
    /*
     * parse argv 
     * more detail visit: http://www.gnu.org/software/libc/manual/html_node/Getopt.html
     */

    if (argc == 1) {
        usage();
        return 0;                    
    }

    while ((opt=getopt_long(argc, argv,"Vc:?h",long_options,&options_index)) != EOF) {
        switch (opt) {
            case  0 : 
                break;
            case 'c':
                conf = optarg;
                break;
            case 'V':
                return 0;
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
    char conf_buf[BUFLEN];
    xm_conf_t cf;
    rc = read_conf(conf, &cf, conf_buf, BUFLEN);
    check(rc == XM_CONF_OK, "read_conf");

    /*
    * install signal handle for SIGPIPE
    * when a fd is closed by remote, writing to this fd will cause system send
    * SIGPIPE to this process, which exit the program
    */
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, NULL)) {
        log_err("install sigal handler for SIGPIPE failed");
        return 0;

    }



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

    log_info("Xserver start");
    int i, n, fd;

    while(1){
        n = xm_epoll_wait(epfd, events, MAXEVENTS, 0);
        
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

                do_request(events[i].data.ptr);
            }
        }
    }

    return 0;
}
    






