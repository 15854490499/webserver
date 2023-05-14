#pragma once
#include <memory>
#include <string>
#include "FileUtil.h"
#include "../lock/locker.h"
#include "noncopyable.h"

class LogFile : noncopyable {

public:
	LogFile(const std::string& basename, int flushEveryN = 1024);
	~LogFile();
	void append(const char *logline, int len);
	void flush();
	bool rollFile();
private:
	void append_unlocked(const char *logfile, int len);
	const std::string basename_;
	const int flushEveryN_;
	int count_;
	std::unique_ptr<locker> locker_;
	std::unique_ptr<AppendFile> file_;
};

