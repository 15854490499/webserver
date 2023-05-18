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
	  cond_(lock_),
	  latch_(1) {
	emptyBuffers.resize(128);
	for(int i = 0; i < 128; i++)
		emptyBuffers[i] = (std::shared_ptr<Buffer>)(new Buffer);
	assert(logFileName_.size() > 1);
}

void AsyncLogging::append(const char* logline, int len) {
	lock_.lock();
	if(!currentBuffer_) {
		if(!emptyBuffers.empty()) {
			currentBuffer_ = std::move(emptyBuffers.front());
			emptyBuffers.pop_front();
			currentBuffer_->bzero();
		}
		else {
			lock_.unlock();
			return;
		}
	}
	if(currentBuffer_->avail() > len) {
		currentBuffer_->append(logline, len);
	}
	else {
		fullBuffers.push_back(currentBuffer_);
		currentBuffer_.reset();
		if(!emptyBuffers.empty()) {
			currentBuffer_ = std::move(emptyBuffers.front());
			emptyBuffers.pop_front();
			currentBuffer_->bzero();
		}		
		else {
			lock_.unlock();
			return;
		}
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
	BufferPtr bufferToWrite;
	BufferVector buffersToWrite;
	buffersToWrite.reserve(16);
	while(running_) {
		assert(buffersToWrite.empty());
		{
			lock_.lock();
			if(fullBuffers.empty()) {
				cond_.timewait(flushInterval_);
				fullBuffers.push_back(currentBuffer_);
			}
			bufferToWrite = std::move(fullBuffers.front());
			fullBuffers.pop_front();
			buffersToWrite.push_back(bufferToWrite);
			if(!emptyBuffers.empty()) {
				currentBuffer_ = std::move(emptyBuffers.front());
				emptyBuffers.pop_front();
				currentBuffer_->bzero();
			}
			lock_.unlock();
		}
		for(size_t i = 0; i < buffersToWrite.size(); ++i) {
			output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
			buffersToWrite[i]->reset();
			emptyBuffers.push_front(std::move(buffersToWrite[i]));
		}
		buffersToWrite.clear();
		output.flush();
	}
	output.flush();
}
