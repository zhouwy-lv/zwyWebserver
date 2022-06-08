#ifndef MIN_HEAP
#define MIN_HEAP

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unordered_map>

#include <time.h>
#include "../log/log.h"

using std::exception;

class util_timer;

struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer
{
public:
    util_timer() {}

public:
    time_t expire;
    
    void (* cb_func)(client_data *);
    client_data *user_data;
};


class time_heap
{
public:
    time_heap()
    {
        capacity = 10;
        cur_size = 0;
        init(capacity);
    }
    
    time_heap(util_timer** init_array, int size, int capacity) throw (std::exception);
    ~time_heap();

    void init(int cap) throw (std::exception);

    void add_timer(util_timer* timer) throw (std::exception);
    util_timer* top() const;
    void pop_timer();
    void del_timer(util_timer* timer);
    void adjust_timer(util_timer* timer);
    void tick();

    bool empty() const { return cur_size == 0;}

private:
    void percolate_down(int hole);
    void resize() throw (std::exception);

    util_timer** array;
    int capacity;
    int cur_size;
    unordered_map<util_timer*, int> timerLocation;
};


class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    time_heap m_timer_heap;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data *user_data);

#endif