#include "scheduler.h"
#include "unistd.h"

static thread_local Scheduler *t_scheduler = nullptr;
static thread_local Fiber *t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, const std::string &name) {
	assert(threads > 0);
	m_name = name;
	m_threadCount = threads;
}

Scheduler *Scheduler::GetThis() {
	return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
	return t_scheduler_fiber;
}

void Scheduler::setThis() {
	t_scheduler = this;
	return;
}

Scheduler::~Scheduler() {
	assert(m_stopping);
	if(GetThis() == this) {
		t_scheduler = nullptr;
	}
}

void Scheduler::start() {
	m_locker.lock();
	if(m_stopping) {
		std::cout << "Scheduler is stopping" << endl;
		return;
	}
	assert(m_threads.empty());
	m_threads.resize(m_threadCount);
	for(size_t i = 0; i < m_threadCount; i++) {
		m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
		m_threadIds.push_back(m_threads[i]->getId());
	}
}

bool Scheduler::stopping() {
	locker.lock();
	return m_stopping && m_tasks.empty() && m_activeThreadCount == 0;
}

void Scheduler::tickle() {
	std::cout << "tickle" << endl;
}

void Scheduler::idle() {
	std::cout << "idle" << endl;
	const uint64_t MAX_EVENTS=256;
	epool_event *event = new epoll_event[MAX_EVENTS]();
	std::shared_ptr<epoll_event> shared_evets(events, [](epoll_event *ptr) {
		delete[] ptr;	
	});
	while(true) {
		if(stopping()) {
			break;
		}
		static const int MAX_TIMEOUT = 5000;
		int rt = epoll_wait()
	}

}

void Scheduler::stop() {
	std::cout << "stop" << endl;;
	if(stopping()) {
		return;
	}
	m_stopping = true;
	assert(GetThis != this);
	for(size_t i = 0; i < m_threadCount; i++) {
		tickle();
	}
	if(m_rootFiber) {
		tickle();
	}
	std::vector<Thread::ptr> thrs;
    {
        m_locker.lock();
        thrs.swap(m_threads);
    }
    for (auto &i : thrs) {
        i->join();
    }
}

void Scheduler::run() {
	std::cout << "run" << endl;
	setThis();
	if(GetThreadId() != m_rootThread) {
		t_scheduler_fiber = Fiber::GetThis().get();
	}
	Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
	SchedulerTask task;
	 while (true) {
        task.reset();
        bool tickle_me = false; // 是否tickle其他线程进行任务调度
        {
            m_locker.lock(m_mutex);
            auto it = m_tasks.begin();
            // 遍历所有调度任务
            while (it != m_tasks.end()) {
                if (it->thread != -1 && it->thread != gettid()) {
                    // 指定了调度线程，但不是在当前线程上调度，标记一下需要通知其他线程进行调度，然后跳过这个任务，继续下一个
                    ++it;
                    tickle_me = true;
                    continue;
                }

                // 找到一个未指定线程，或是指定了当前线程的任务
                assert(it->fiber);
                if (it->fiber) {
                    // 任务队列时的协程一定是READY状态，谁会把RUNNING或TERM状态的协程加入调度呢？
                    assert(it->fiber->getState() == Fiber::READY);
                }
                // 当前调度线程找到一个任务，准备开始调度，将其从任务队列中剔除，活动线程数加1
                task = *it;
                m_tasks.erase(it++);
                ++m_activeThreadCount;
                break;
            }
            // 当前线程拿完一个任务后，发现任务队列还有剩余，那么tickle一下其他线程
            tickle_me |= (it != m_tasks.end());
        }

        if (tickle_me) {
            tickle();
        }

        if (task.fiber) {
            // resume协程，resume返回时，协程要么执行完了，要么半路yield了，总之这个任务就算完成了，活跃线程数减一
            task.fiber->resume();
            --m_activeThreadCount;
            task.reset();
        } else {
            // 进到这个分支情况一定是任务队列空了，调度idle协程即可
            if (idle_fiber->getState() == Fiber::TERM) {
                // 如果调度器没有调度任务，那么idle协程会不停地resume/yield，不会结束，如果idle协程结束了，那一定是调度器停止了
				std::cout << "idle fiber term";
                break;
            }
            ++m_idleThreadCount;
            idle_fiber->resume();
            --m_idleThreadCount;
        }
    }
    std::cout << "Scheduler::run() exit";
}

