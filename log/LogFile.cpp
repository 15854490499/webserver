#include "LogFile.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "FileUtil.h"

using namespace std;

LogFile::LogFile(const string& basename, int flushEveryN) 
	: basename_(basename), 
	  flushEveryN_(flushEveryN),
	  count_(0),
	  locker_(new locker) {
	file_.reset(new AppendFile(basename));
}

LogFile::~LogFile() {}

void LogFile::append(const char* logline, int len) {
	locker_->lock();
	append_unlocked(logline, len);
	locker_->unlock();
}

void LogFile::flush() {
	locker_->lock();
	file_->flush();
	locker_->unlock();
}

void LogFile::append_unlocked(const char* logline, int len) {
	file_->append(logline, len);
	++count_;
	if(count_ >= flushEveryN_) {
		count_ = 0;
		file_->flush();
	}
}
