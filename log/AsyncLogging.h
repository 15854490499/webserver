#pragma once
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <deque>
#include "CountDownLatch.h"
#include "LogStream.h"
#include "../lock/locker.h"
#include "noncopyable.h"

class AsyncLogging : noncopyable {

public:
	AsyncLogging(const std::string basename, int flushInterval = 2);
	~AsyncLogging() {
		if(running_) stop();
	}
	void append(const char* logline, int len);

	void start() {
		running_ = true;
		pthread_create(&thread_, NULL, &worker, this);
		latch_.wait();
	}

	void stop() {
		running_ = false;
		cond_.signal();
		pthread_join(thread_, NULL);
	} 
private:
	void threadFunc();
	static void* worker(void* arg); 
	typedef FixedBuffer<kLargeBuffer> Buffer;
	typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
	typedef std::deque<std::shared_ptr<Buffer>> Stack;
	typedef std::deque<std::shared_ptr<Buffer>> Queue;
	typedef std::shared_ptr<Buffer> BufferPtr;
	const int flushInterval_;
	bool running_;
	std::string basename_;
	pthread_t thread_;
	locker lock_;
	cond cond_;
	BufferPtr currentBuffer_;
	Queue fullBuffers;
	Stack emptyBuffers;
	CountDownLatch latch_;
};
