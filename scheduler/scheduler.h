#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include <iostream>
#include "../fiber/fiber.h"
#include "../lock/lock.h"
#include "../threadpool/threadpool.h"

class Scheduler {
public:
	typedef std::shared_ptr<Scheduler> ptr;
	Scheduler(size_t threads = 1, const std::string& name = "");
	virtual ~Scheduler();
	const std::string& getName() const { return m_name;}
	static Scheduler* GetThis();
	static Fiber* GetMainFiber();
	void start();
	void stop();
	template<class Fiber>
	void schedule(Fiber f, int thread = -1) {
		bool need_tictle = false;
		{
			my_locker.lock();
			need_tickle = scheduleNolock(f, thread);
		}
		if(need_tickle)
			tickle();
	}
protected:
	virtual void tickle();
	void run();
	virtual void idle();
	virtual bool stopping();
	void setThis();
private:
	template<class Fiber>
	bool scheduleNoLock(Fiber f, int thread) {
		bool need_tickle = m_tasks.empty();
		SchedulerTask task(f, thread);
		if(task.fiber) {
			m_tasks.push_back(task);
		}
		return need_tickle;
	}
private:
	struct SchedulerTask {
		Fiber::ptr fiber;
		int thread;
		SchedulerTask(Fiber::ptr f, int thr) {
			fiber = f;
			thread = thr;
		}
		SchedulerTask(Fiber::ptr *f, int thr) {
			fiber.swap(*f);
			thread = f;
		}
		Schedulertask() { thread = -1; }
		void reset() {
			fiber = nullptr;
			thread = -1;
		}
	}
private:
	locker my_locker;
	std::string m_name;
	std::vector<Thread::ptr> m_threads;
	std::list<SchedulerTask> m_tasks;
	std::vector<int> m_threadIds;
	size_t m_threadCount = 0;
	std::atomic<size_t> m_activeThreadCount = {0};
	std::atomic<size_t> m_idleThreadCount = {0};
	/*bool m_useCaller;
	Fiber::ptr m_rootFiber;*/
	int m_rootThread = 0;
	bool m_stopping = false;
};
