#include "lst_timer.h"
#include <sys/syscall.h>
#include "../log/log.h"
#include <limits.h>
#include "../http/http_conn.h"

sort_timer_heap::sort_timer_heap()
{
	printf("%s\n", "create timer_heap begin");
    capacity = 1000;
	curSize = 0;
	array = new util_timer* [capacity];
	if(array == NULL)
	{
		std::cerr << "heap startup fail!" << endl;
		exit(1);
	}
	for(int i = 0; i < capacity; i++)
	{
		array[i] = NULL;
	}
	printf("%s\n", "create timer_heap end");
}

sort_timer_heap::~sort_timer_heap()
{
	for(int i = 0; i < curSize; i++) 
	{
		delete array[i];
	}
    delete[] array;
}

void sort_timer_heap::siftDown(int start, int m) {
	//std::cout << "siftdown" << " " << start << " "<< m << endl;
	int i = start, j = 2 * i + 1;
	util_timer* t = array[i];
	while(j <= m)
	{
		if(j < m && array[j+1]->expire < array[j]->expire)
			j++;
		if(array[j]->expire > t->expire) break;
		else {
			array[i] = array[j];
			array[i]->idx = i;
		}
		i = j;
		j = 2 * i + 1;
	}
	array[i] = t;
	array[i]->idx = i;
	return;
}

void sort_timer_heap::siftUp(int start) {
	//std::cout << "siftup" << " " << start << " "<< curSize << endl;
	int j = start, i = (j - 1) / 2;
	util_timer* t = array[j];
	/*if(array[j] == NULL)
		std::cout << "NULLLLLLLLLLLLLLLLLLLLLLLL" << endl;*/
	while(j > 0)
	{
		if(array[i]->expire < t->expire) break;
		else {
			array[j] = array[i];
			array[j]->idx = j;
		}
		j = i;
		i = (j - 1) / 2;
	}
	array[j] = t;
	array[j]->idx = j;
	
	return;
}

void sort_timer_heap::resize() throw (std::exception)
{
	//printf("%s\n", "resize timer_heap once");
	util_timer** temp = new util_timer* [2 * capacity];
	for(int i = 0; i < 2*capacity; i++)
	{
		temp[i] = NULL;
	}
	if(!temp)
	{
		throw std::exception();
	}
	capacity = 2 * capacity;
	for(int i = 0; i < curSize; i++)
	{
		temp[i] = array[i];
	}
	delete[] array;
	array = temp;
	//printf("%s\n", "resize timer_heap end");
	return;
}

void sort_timer_heap::add_timer(util_timer *timer)
{
	//printf("%s\n", "add timer once");
	//cout << curSize << endl;
	if(curSize == capacity)
	{
		std::cout << "Heap Full!" << endl;
		resize();
	}
	array[curSize] = timer;
	siftUp(curSize);
	curSize++;
	//printf("%s\n", "add timer end");
	return;
}

void sort_timer_heap::adjust_timer(util_timer *timer)
{
	//printf("%s\n", "adjust timer_heap once");
	if(!timer)
		return;
	int start = timer->idx;
	int expire = timer->expire;
	timer->expire = INT_MIN;
	siftUp(start);
	array[0] = array[--curSize];
	siftDown(0, curSize);
	timer->expire = expire;
	add_timer(timer);
	//printf("%s\n", "adjust timer_heap end");
	return;
}

void sort_timer_heap::pop_timer()
{
	//printf("%s\n", "pop timer_heap begin");
	//cout << curSize << endl;
	if(curSize == 0)
		return;
	delete array[0];
	curSize--;
	if(curSize > 0)
	{
		array[0] = array[curSize];
		siftDown(0, curSize - 1);
	}
	else
		array[0] = NULL;
	//printf("%s\n", "pop timer_heap end");
	return;
}

void sort_timer_heap::del_timer(util_timer *timer)
{
	//printf("%s\n", "del timer begin");
	if(!timer)
		return;
	int start = timer->idx;
	int expire = timer->expire;
	timer->expire = INT_MIN;
	siftUp(start);
	pop_timer();
	//printf("%s\n", "del timer end");
	return;
}

void sort_timer_heap::tick(int cur)
{
	//std::cout << "tick begin" << endl;
	util_timer* tmp = array[0];
	//time_t cur = time(NULL);
	while(curSize > 0)
	{
		if(!tmp)
			break;
		if(tmp->expire > cur)
			break;
		if(array[0]->cb_func)
		{
			array[0]->cb_func(array[0]->user_data);
		}
		pop_timer();
		tmp = array[0];
	}
	//std::cout << "tick end" << endl;
	return;
}

bool sort_timer_heap::isEmpty() {
	return curSize == 0;
}

util_timer* sort_timer_heap::top() {
	if(isEmpty()) {
		return NULL;
	}
	return array[0];
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
	time_t t = time(NULL);
    m_timer_heap.tick(t);
    //alarm(m_TIMESLOT);
	alarm(getNextTick(t));
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int Utils::getNextTick(int t) {
	int ret = m_TIMESLOT;
	if(!m_timer_heap.isEmpty()) {
		ret = m_timer_heap.top()->expire - t;
		std::cout << ret << endl;
		if(ret < 0)
			ret = 0;
	}
	return ret;
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data* user_data)
{
	//std::cout << syscall(__NR_gettid) << "call back" << endl;
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
	//std::cout << "call back end" << endl;
}
