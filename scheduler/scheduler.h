#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include <iostream>
#include <functional>
#include <atomic>
#include "../fiber/fiber.h"
#include "../log/macro.h"
#include "../lock/locker.h"

class Scheduler {
public:
	typedef std::shared_ptr<Scheduler> ptr;
	Scheduler(size_t threads = 1);
	virtual ~Scheduler();
	static Scheduler* GetThis();
	static Fiber* GetMainFiber();
	template<class FiberOrCb>
	void schedule(FiberOrCb fc, int thread = -1) {
		bool need_tickle = false;
		{
			m_locker.lock();
			need_tickle = scheduleNoLock(fc, thread);
			m_locker.unlock();
		}
		if(need_tickle)
			tickle();
	}
protected:
	virtual void tickle();
	void run();
	virtual void idle();
	void setThis();
	bool hasIdleThreads() { return m_idleThreadCount > 0; }
private:
	template<class FiberOrCb>
	bool scheduleNoLock(FiberOrCb fc, int thread) {
		bool need_tickle = m_tasks.empty();
		SchedulerTask task(fc, thread);
		if(task.fiber || task.cb) {
			m_tasks.push_back(task);
		}
		return need_tickle;
	}
private:
	struct SchedulerTask {
		Fiber::ptr fiber;
		std::function<void()> cb;
		int thread;
		SchedulerTask(Fiber::ptr f, int thr) {
			fiber = f;
			thread = thr;
		}
		SchedulerTask(Fiber::ptr *f, int thr) {
			fiber.swap(*f);
			thread = thr;
		}
		SchedulerTask(std::function<void()> f, int thr) {
			cb = f;
			thread = thr;
		}
		SchedulerTask() { thread = -1; }
		void reset() {
			fiber = nullptr;
			cb = nullptr;
			thread = -1;
		}
	};
private:
	locker m_locker;
	std::list<SchedulerTask> m_tasks;
	size_t m_threadCount = 0;
	std::atomic<size_t> m_activeThreadCount = {0};
	std::atomic<size_t> m_idleThreadCount = {0};
	Fiber::ptr m_rootFiber;
	int m_rootThread = 0;
};
#endif
