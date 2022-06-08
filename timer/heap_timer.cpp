#include "heap_timer.h"
#include "../http/http_conn.h"

void time_heap::init(int cap) throw( std:: exception)
{
    array = new util_timer* [capacity];
    if(!array)
    {
        // LOG_ERROR("heap_time create error");
        throw std::exception();
    }

    for(int i = 0; i < capacity; i ++)
    {
        array[i] = NULL;
    }
}

time_heap::time_heap(util_timer** init_array, int size, int capacity) throw (std::exception) : cur_size(size), capacity(capacity)
{
    if(capacity < size)
    {
        // LOG_ERROR("head_time array don't have egough capacity");
        throw std::exception();
    }
    array = new util_timer* [capacity];
    if(!array)
    {
        // LOG_ERROR("heap_time create error");
        throw std::exception();
    }

    for(int i = 0; i < capacity; i ++)
    {
        array[i] = NULL;
    }
    if(size != 0)
    {
        for(int i = 0; i < size; i ++)
        {
            array[i] = init_array[i];
        }
        for(int i = (cur_size - 1) / 2; i >= 0; i --)
        {
            percolate_down(i);
        }
    }

}

time_heap::~time_heap()
{
    for(int i = 0; i  < cur_size; i ++)
    {
        delete array[i];
    }
    delete [] array;
}

void time_heap::add_timer(util_timer* timer) throw (std::exception)
{
    if( !timer )
    {
        return;
    }
    if(cur_size >= capacity)
    {
        resize();
    }
    int hole = cur_size ++;
    int parent = 0;
    for(; hole > 0; hole = parent)
    {
        parent = (hole - 1) / 2;
        if(array[parent] -> expire <= timer -> expire)
        {
            break;
        }
        array[hole] = array[parent];
        timerLocation[array[parent]] = hole;
    }
    array[hole] = timer;
    timerLocation[timer] = hole;
}

void time_heap::del_timer( util_timer* timer)
{
    if( !timer )
    {
        return;
    }

    timer -> cb_func = NULL;
    timerLocation.erase(timer);
}

util_timer* time_heap::top() const
{
    if( empty() )
    {
        return NULL;
    }
    return array[0];
}

void time_heap::pop_timer()
{
    if(empty())
    {
        return;
    }
    if( array[0] )
    {
        delete array[0];
        timerLocation.erase(array[0]);
        array[0] = array[-- cur_size];

        percolate_down(0);
    }
}
void time_heap::adjust_timer(util_timer* timer)
{
    if (!timer)
    {
        return;
    }
    // for( int i = 0; i < cur_size; i ++){
    //     if(array[i] == timer){
    //         percolate_down(i);
    //     }
    // }
    if(timerLocation.find(timer) != timerLocation.end()){
        percolate_down(timerLocation[timer]);
    }else{
        std::cout << "timer location don't find" << endl;
        return;
    }
    
}


void time_heap::tick()
{
    util_timer* tmp = array[0];
    if (!tmp)
    {
        return;
    }
    time_t cur = time( NULL );
    while( !empty() )
    {
        if( !tmp )
        {
            break;
        }

        if(tmp -> expire > cur)
        {
            break;
        }

        if( array[0] -> cb_func )
        {
            array[0] -> cb_func( array[0] -> user_data );
        }
        pop_timer();
        tmp = array[0];
    }
}

void time_heap::percolate_down( int hole )
{
    util_timer* temp = array[hole];
    int child = 0;
    for( ; ((hole*2 + 1) <= (cur_size - 1)); hole = child )
    {
        child = 2*hole + 1;
        if( (child < (cur_size - 1)) && (array[child + 1] -> expire < array[child] -> expire) )
        {
            ++ child;
        }
        if( array[child] -> expire < temp -> expire )
        {
            array[hole] = array[child];
            timerLocation[array[child]] = hole;
        }
        else
        {
            break;
        }
    }
    array[hole] = temp;
    timerLocation[temp] = hole;
}

void time_heap::resize() throw ( std::exception )
{
    util_timer** temp = new util_timer*[2*capacity];
    for(int i = 0; i < 2*capacity; i ++)
    {
        temp[i] = NULL;
    }
    if( !temp )
    {
        throw std::exception();
    }
    capacity = 2*capacity;
    for(int i = 0; i < cur_size; i ++)
    {
        temp[i] = array[i];
    }
    delete [] array;
    timerLocation.clear();
    array = temp;
    for(int i = 0; i < cur_size; i ++){
        timerLocation[array[i]] = i;
    }
}


void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_heap.tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}
