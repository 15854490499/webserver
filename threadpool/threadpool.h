#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cstdio>
#include <exception>
#include <pthread.h>
#include <sys/unistd.h>
#include <sys/syscall.h>
#include "../http/http_conn.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../scheduler/scheduler.h"

template <typename T>
class threadpool : public Scheduler
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(int actor_model, connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
	//向请求队列中插入任务请求
    bool append(T *request, int state);
    bool append_p(T *request);
private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void *worker(void *arg);
	void handleClient(T*);
private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //描述线程池的数组，其大小为m_thread_number
    connection_pool *m_connPool;  //数据库
    int m_actor_model;          //模型切换
	int m_epfd = 0;
	int m_pipe[2];
protected:
	void idle() override;
	void tickle() override;
};
template <typename T>
threadpool<T>::threadpool( int actor_model, connection_pool *connPool, int thread_number, int max_requests) : Scheduler(thread_number)
{
	m_actor_model = actor_model;
	m_thread_number = thread_number;
	m_max_requests = max_requests;
	m_connPool = connPool;
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads)
        throw std::exception();
	//创建epoll
	m_epfd = epoll_create(5000);
	LOG_INFO("threadpoll创建%d", m_epfd);
	int rt = pipe(m_pipe);
	assert(!rt);
	//关注可读事件
	epoll_event event;
	memset(&event, 0, sizeof(epoll_event));
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = m_pipe[0];
	//非阻塞方式，配合边缘触发。
	rt = fcntl(m_pipe[0], F_SETFL, O_NONBLOCK);
	assert(!rt);
	//监听内核事件
	rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_pipe[0], &event);
	assert(!rt);

	//创建线程 
	for (int i = 0; i < thread_number; ++i)
    {
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }

}
template <typename T>
threadpool<T>::~threadpool()
{
	close(m_epfd);
	close(m_pipe[0]);
	close(m_pipe[1]);
    delete[] m_threads;
}
template <typename T>
bool threadpool<T>::append(T *request, int state)
{
    request->m_state = state;
	/*write(m_pipe[1], &request, sizeof(request));*/
	schedule(bind(&threadpool::handleClient, this, request));
    return true;
}
template <typename T>
bool threadpool<T>::append_p(T *request)
{
   	//write(m_pipe[1], &request, sizeof(request));
	schedule(bind(&threadpool::handleClient, this, request));
    //m_queuestat.post();
    return true;
}

template <typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::tickle() {
    if(!hasIdleThreads()) {
        return;
    }
    int rt = write(m_pipe[1], "T", 1);
    LOG_ASSERT(rt == 1, "tickle assertion");
}

template <typename T>
void threadpool<T>::idle() {
	const uint64_t MAX_EVENTS = 256;
	epoll_event *events = new epoll_event[MAX_EVENTS]();
	std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr) {
        delete[] ptr;
    });
	while(true) {
		//std::cout << syscall(__NR_gettid) << "idle" << endl;
		//static const int MAX_TIMEOUT = 5000;
		int rt = epoll_wait(m_epfd, events, MAX_EVENTS, -1/*, MAX_TIMEOUT*/);
		if(rt < 0) {
			if(errno == EINTR) {
				continue;
			}
			LOG_ERROR("epoll err");
			break;
		}
		bool flag = false;
		for(int i = 0; i < rt; i++) {
			epoll_event &event = events[i];
			if(event.data.fd == m_pipe[0]) {
				flag = true;
				uint8_t dummy[256];
				while(read(m_pipe[0], dummy, sizeof(dummy)) > 0);
				continue;
			}
		}
		if(flag)
		{
			Fiber::ptr cur = Fiber::GetThis();
			auto raw_ptr = cur.get();
			cur.reset();
			raw_ptr->swapOut();
		}
	}
}
template <typename T>
void threadpool<T>::handleClient(T* request) {
		if (1 == m_actor_model)
        {
            if (0 == request->m_state)
            {
                if (request->read_once())
                {
                    request->improv = 1;
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
					request->process();
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
            else
            {
                if (request->write())
                {
                    request->improv = 1;
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        else
        {
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }
}
#endif
