#pragma once
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "LogStream.h"

class AsyncLogging;

class Logger {
public:
	enum Level {
		UNKNOWN = 0,
		DEBUG = 1,
		INFO = 2,
		WARN = 3,
		ERROR = 4,
		FATAL = 5
	};
	Logger(const char* fileName, int line, int level);
	~Logger();
	LogStream &stream() { return impl_.stream_; }
	static void setLogFileName(std::string fileName) { logFileName_ = fileName; }
	static std::string getLogFileName() { return logFileName_; }
private:
	class Impl {
	public:
		Impl(const char* fileName, int line, int level);
		void formatTime();
		LogStream stream_;
		int line_;
		int level_;
		std::string basename_;
	};
	Impl impl_;
	static std::string logFileName_;
};
#define LOG(level) Logger(__FILE__, __LINE__, level).stream()
#define LOG_DEBUG LOG(Logger::DEBUG)
#define LOG_INFO LOG(Logger::INFO)
#define LOG_WARN LOG(Logger::WARN)
#define LOG_ERROR LOG(Logger::ERROR)
#define LOG_FATAL LOG(Logger::FATAL)
#define LOG_UNKNOWN LOG(Logger::UNKNOWN)
