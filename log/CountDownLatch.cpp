#include "CountDownLatch.h"
#include <iostream>
CountDownLatch::CountDownLatch(int count) : lock_(), cond_(lock_), count_(count) {}

void CountDownLatch::wait() {
	lock_.lock();
	while(count_ > 0) 
		cond_.wait();
	lock_.unlock();
}

void CountDownLatch::countDown() {
	lock_.lock();
	--count_;
	if(count_ == 0) cond_.broadcast();
	lock_.unlock();
}
