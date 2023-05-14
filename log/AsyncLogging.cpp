#include "AsyncLogging.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include "LogFile.h"

AsyncLogging::AsyncLogging(const std::string logFileName_, int flushInterval) 
	: flushInterval_(flushInterval),
	  running_(false),
	  basename_(logFileName_),
	  currentBuffer_(new Buffer),
	  nextBuffer_(new Buffer),
	  buffers_(),
	  cond_(lock_),
	  latch_(1) {
	assert(logFileName_.size() > 1);
	currentBuffer_->bzero();
	nextBuffer_->bzero();
	buffers_.reserve(16);
}

void AsyncLogging::append(const char* logline, int len) {
	lock_.lock();
	if(currentBuffer_->avail() > len)
		currentBuffer_->append(logline, len);
	else {
		buffers_.push_back(currentBuffer_);
		currentBuffer_.reset();
		if(nextBuffer_) 
			currentBuffer_ = std::move(nextBuffer_);
		else
			currentBuffer_.reset(new Buffer);
		currentBuffer_->append(logline, len);
		cond_.signal();
	}
	lock_.unlock();
}

void *AsyncLogging::worker(void* arg) {
	AsyncLogging *al = (AsyncLogging*)arg;
	al->threadFunc();
	return al;
}

void AsyncLogging::threadFunc() {
	assert(running_ == true);
	latch_.countDown();
	LogFile output(basename_);
	BufferPtr newBuffer1(new Buffer);
	BufferPtr newBuffer2(new Buffer);
	newBuffer1->bzero();
	newBuffer2->bzero();
	BufferVector buffersToWrite;
	buffersToWrite.reserve(16);
	while(running_) {
		assert(newBuffer1 && newBuffer1->length() == 0);
		assert(newBuffer2 && newBuffer2->length() == 0);
		assert(buffersToWrite.empty());
		{
			lock_.lock();
			if(buffers_.empty()) {
				cond_.timewait(flushInterval_);
			}
			buffers_.push_back(currentBuffer_);
			currentBuffer_.reset();
			currentBuffer_ = std::move(newBuffer1);
			buffersToWrite.swap(buffers_);
			if(!nextBuffer_) {
				nextBuffer_ = std::move(newBuffer2);
			}
			lock_.unlock();
		}
		assert(!buffersToWrite.empty());
		if(buffersToWrite.size() > 25) {
			buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
		}
		for(size_t i = 0; i < buffersToWrite.size(); ++i) {
			output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
		}
		if(buffersToWrite.size() > 2) {
			buffersToWrite.resize(2);
		}
		if(!newBuffer1) {
			assert(!buffersToWrite.empty());
			newBuffer1 = buffersToWrite.back();
			buffersToWrite.pop_back();
			newBuffer1->reset();
		}
		if(!newBuffer2) {
			assert(!buffersToWrite.empty());
			newBuffer2 = buffersToWrite.back();
			buffersToWrite.pop_back();
			newBuffer2->reset();
		}
		buffersToWrite.clear();
		output.flush();
	}
	output.flush();
}
