#include "fiber.h"
#include "../webserver.h"
#include <string.h>
#include <atomic>
#include <exception>


static std::atomic<uint64_t> gFiberId {0};
static std::atomic<uint64_t> gFiberCnt {0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_thread_fiber = nullptr;

uint64_t getStackSize()
{
    static uint64_t size = 1024 * 1024;
    if (size == 0) {
        return 1024 * 1024;
    }
    return size;
}

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

Fiber::Fiber() {
	m_state = EXEC;
	SetThis(this);
	if(getcontext(&m_ctx)) {
		LOG_ERROR("getcontext error %s", strerror(errno));
	}
	++gFiberCnt;
	//std::cout << "Fiber::Fiber main start id =, total =" << m_id << gFiberCnt.load() << endl;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize) 
	: m_id(++gFiberId)
	, m_cb(cb) {
		++gFiberCnt;
		m_stacksize = stacksize ? stacksize : getStackSize();
		m_stack = StackAllocator::Alloc(m_stacksize);
		if(getcontext(&m_ctx)) {
			LOG_ERROR("getcontext error %d %s", errno, strerror(errno));
			exit(0);
		}
		m_ctx.uc_link = nullptr;
		m_ctx.uc_stack.ss_sp = m_stack;
		m_ctx.uc_stack.ss_size = m_stacksize;
		makecontext(&m_ctx, &Fiber::FiberEntry, 0);
		//std::cout << "Fiber::Fiber id= " << m_id << endl;
}

Fiber::~Fiber() {
	--gFiberCnt;
	if(m_stack) {
		LOG_ASSERT(m_state == TERM, "Fiber State assertion");
		StackAllocator::Dealloc(m_stack, m_stacksize);
	} else {
		assert(!m_cb);
		assert(m_state == EXEC);
		Fiber* cur = t_fiber;
		if(cur == this)
		{
			SetThis(nullptr);
		}
	}
}

void Fiber::reset(std::function<void()> cb) {
	LOG_ASSERT(m_stack, "Fiber Stack assertion")
	LOG_ASSERT(m_state == TERM, "Fiber State assertion");
	m_cb = cb;
	if(getcontext(&m_ctx)) {
		LOG_ERROR("getcontext error %d %s", errno, strerror(errno));
		exit(0);
	}
	m_ctx.uc_link = nullptr;
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = m_stacksize;
	makecontext(&m_ctx, &Fiber::FiberEntry, 0);
	m_state = READY;
}

//切换到当前协程执行
void Fiber::swapIn() {
    SetThis(this);
    LOG_ASSERT(m_state != EXEC, "Fiber State assertion");
    m_state = EXEC;
    if(swapcontext(&t_thread_fiber->m_ctx, &m_ctx)) {
		LOG_ERROR("swapcontext error %d %s", errno, strerror(errno));
		exit(0);    
	}
}

//切换到后台执行
void Fiber::swapOut() {
    LOG_ASSERT(m_state == EXEC || m_state == TERM, "Fiber State assertion");
    SetThis(t_thread_fiber.get());
	if(m_state != TERM) {
		m_state = READY;
	}
    if(swapcontext(&m_ctx, &t_thread_fiber->m_ctx)) {
		LOG_ERROR("swapcontext error %d %s", errno, strerror(errno));
		exit(0);    
    }
}

//设置当前协程
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

/*返回当前协程, 如果当前线程还未创建协程，则创建线程的第一个协程，
     * 且该协程为当前线程的主协程，其他协程都通过这个协程来调度，也就是说，其他协程
     * 结束时,都要切回到主协程，由主协程重新选择新的协程进行resume
     * 线程如果要创建协程，那么应该首先执行一下Fiber::GetThis()操作，以初始化主函数协程
	 */
Fiber::ptr Fiber::GetThis() {
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    LOG_ASSERT(t_fiber == main_fiber.get(), "running Fiber assertion");
    t_thread_fiber = main_fiber;
    return t_fiber->shared_from_this();
}


uint64_t Fiber::TotalFibers() {
	return gFiberCnt;
}

void Fiber::FiberEntry()
{
    Fiber::ptr curr = GetThis();
    LOG_ASSERT(curr != nullptr, "running Fiber assertion");
    curr->m_cb();
    curr->m_cb = nullptr;
    curr->m_state = TERM;

    Fiber *ptr = curr.get();
    curr.reset();
    ptr->swapOut();
    LOG_ASSERT(false, "never reach here");
}

