#pragma once
#include "../lock/locker.h"
#include "noncopyable.h"

class CountDownLatch : noncopyable {
public: 
	explicit CountDownLatch(int count);
	void wait();
	void countDown();
private:
	mutable locker lock_;
	cond cond_;
	int count_;
};
