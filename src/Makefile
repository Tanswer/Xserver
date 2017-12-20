objects = xm_main.o rio.o xm_http.o xm_http_parse.o \
		               xm_http_request.o epoll.o util.o
 
edit : $(objects)
	cc -o xmserver $(objects) 
xm_main.o : epoll.h util.h xm_http.h xm_http_request.h dbg.h timer.h
rio.o : rio.h
xm_http.o : xm_http.h xm_http_request.h dbg.h timer.h epoll.h rio.h
xm_http_request.o : xm_http_request.h list.h
xm_http_parse.o : xm_http.h dbg.h
epoll.o : epoll.h dbg.h
util.o : util.h dbg.h

clean :
	rm edit 
	rm -f *.o
