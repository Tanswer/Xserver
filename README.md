----------
### Programming Model
 1. I/O multiplexing(epoll ET)
 2. Non-blocking I/O 、finite state machine
 3. threadpool

### How To Run
please make sure you have [cmake](https://cmake.org/) installed.
```
git clone git@github.com:Tanswer/Xserver.git

cd Xserver

./autorun

```

### Complete

 1. http persistent connection
 2. serve static(.html)
 3. support PHP(FastCGI)
 4. timers(MinHeap)
 3. GET / POST
 4. some HTTP/1.1 features

### To Do

 1. memory pool
 2. keep-alive(timeout)

### webbench

![](http://test-1252727452.costj.myqcloud.com/2018-03-02%2010-26-42%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)


### strace 追踪系统调用
![](http://test-1252727452.costj.myqcloud.com/2018-03-02%2016-03-05%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)
 
