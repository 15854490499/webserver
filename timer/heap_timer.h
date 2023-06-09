#ifndef LST_TIMER
#define LST_TIMER

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

#include <time.h>
#include "../log/log.h"
#include "../fiber/fiber.h"

class util_timer;
//连接资源
struct client_data
{
	//客户端socket地址
    sockaddr_in address;
	//socket文件描述符
    int sockfd;
	//定时器
    util_timer *timer;
};

//定时器类
class util_timer
{
public:
    util_timer(int delay) {expire = time(NULL) + delay;}

public:
	//超时时间
    time_t expire;
    //回调函数
    void (* cb_func)(client_data*);
	//连接资源
    client_data *user_data;
	//定时器在堆中位置
	int idx;
	bool operator<(const util_timer& t) {
		return expire < t.expire;
	}
	//前向定时器
    //util_timer *prev;
	//后继定时器
    //util_timer *next;
};

class sort_timer_heap
{
public:
    sort_timer_heap();
    ~sort_timer_heap();

    void add_timer(util_timer *timer);
	void del_timer(util_timer *timer);
    void adjust_timer(util_timer *timer);
    void pop_timer();
	bool isEmpty();
    void tick(int cur);
	util_timer* top();
private:
	void resize() throw (std::exception);
	void siftDown(int start, int m);
	void siftUp(int start);
private:
    //void add_timer(util_timer *timer, util_timer *lst_head);
	int capacity;
	int curSize;
	util_timer** array;
    //util_timer *head;
    //util_timer *tail;
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
	
	int getNextTick(int t);
public:
    static int *u_pipefd;
    sort_timer_heap m_timer_heap;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data* user_data);

#endif
