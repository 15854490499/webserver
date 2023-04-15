#include "scheduler.h"
#include <sys/unistd.h>
#include <sys/syscall.h>

// 当前线程的调度器，同一个调度器下的所有线程共享同一个实例
static thread_local Scheduler *t_scheduler = nullptr;
// 当前线程的调度协程，每个线程都独有一份
static thread_local Fiber *t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads) {
	LOG_ASSERT(threads > 0, "threads number assertion");
	m_threadCount = threads;
	m_rootThread = -1;
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
	if(GetThis() == this) {
		t_scheduler = nullptr;
	}
}

void Scheduler::tickle() {
	LOG_INFO("tickle");
}
void Scheduler::idle() {
    LOG_DEBUG("idle");
   	return;
}

void Scheduler::run() {
	setThis();
	if(syscall(__NR_gettid) != m_rootThread) {
		t_scheduler_fiber = Fiber::GetThis().get();
	}
	Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
	Fiber::ptr cb_fiber;
	SchedulerTask task;
	while (true) {
        task.reset();
        bool tickle_me = false; // 是否tickle其他线程进行任务调度
        {
            m_locker.lock();
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
                LOG_ASSERT(it->fiber || it->cb, "it->fiber || it->cb task assertion");
                if (it->fiber) {
                    // 任务队列时的协程一定是READY状态，谁会把RUNNING或TERM状态的协程加入调度呢？
                    LOG_ASSERT(it->fiber->getState() == Fiber::READY, "it->fiber->getState() == Fiber::READY task assertion");
                }
                // 当前调度线程找到一个任务，准备开始调度，将其从任务队列中剔除，活动线程数加1
                task = *it;
                m_tasks.erase(it++);
                ++m_activeThreadCount;
                break;
            }
			m_locker.unlock();
            // 当前线程拿完一个任务后，发现任务队列还有剩余，那么tickle一下其他线程
            tickle_me |= (it != m_tasks.end());
        }

        if (tickle_me) {
            tickle();
        }

        if (task.fiber) {
            // resume协程，resume返回时，协程要么执行完了，要么半路yield了，总之这个任务就算完成了，活跃线程数减一
            task.fiber->swapIn();
            --m_activeThreadCount;
            task.reset();
        } else if (task.cb) {
            if (cb_fiber) {
                cb_fiber->reset(task.cb);
            } else {
                cb_fiber.reset(new Fiber(task.cb));
            }
            task.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            cb_fiber.reset(); 
		} else {
            // 进到这个分支情况一定是任务队列空了，调度idle协程即可
            if (idle_fiber->getState() == Fiber::TERM) {
                // 如果调度器没有调度任务，那么idle协程会不停地resume/yield，不会结束，如果idle协程结束了，那一定是调度器停止了
                LOG_INFO("idle fiber term");
                break;
            }
            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
        }
    }
    LOG_INFO("Scheduler::run() exit");
}

