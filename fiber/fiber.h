#ifndef __FIBER_H__
#define __FIBER_H__
#include <memory>
#include <functional>
#include <ucontext.h>
#include <stdio.h>
#include "../log/log.h"

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
	typedef std::shared_ptr<Fiber> ptr;
	enum State {
		READY,
		EXEC,
		TERM,
	};
private:
	Fiber();
public:
	Fiber(std::function<void()> cb, size_t stacksize = 0);
	~Fiber();
	void reset(std::function<void()> cb);
	void swapIn();
	void swapOut();
	uint64_t getId() const { return m_id;}
	State getState() const { return m_state;}
public:
	static void SetThis(Fiber* f);
	static Fiber::ptr GetThis();
	//static void YieldToReady();
	//static void YieldToHold();
	static uint64_t TotalFibers();
	static void FiberEntry();
private:
    /// 协程id
    uint64_t m_id = 0;
    /// 协程运行栈大小
    uint32_t m_stacksize = 0;
    /// 协程状态
    State m_state = READY;
    /// 协程上下文
    ucontext_t m_ctx;
    /// 协程运行栈指针
    void* m_stack = nullptr;
    /// 协程运行函数
	std::function<void()> m_cb;
	int m_close_log;
};

#endif
