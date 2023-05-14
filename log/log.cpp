#include "log.h"
#include "AsyncLogging.h"
#include <assert.h>
#include <iostream>
#include <time.h>
#include <sys/time.h>

static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static AsyncLogging *AsyncLogger_;

std::string Logger::logFileName_ = "";
void once_init() 
{
	AsyncLogger_ = new AsyncLogging(Logger::getLogFileName());
	AsyncLogger_->start();
}

void output(const char* msg, int len) 
{
	pthread_once(&once_control_, once_init);
	AsyncLogger_->append(msg, len);
}

Logger::Impl::Impl(const char* fileName, int line, int level) 
	: stream_(), 
	line_(line), 
	basename_(fileName),
	level_(level)

{
	formatTime();
}

void Logger::Impl::formatTime() 
{
	struct timeval tv;
	time_t time;
	char str_t[26] = {0};
	gettimeofday(&tv, NULL);
	time = tv.tv_sec;
	struct tm* p_time = localtime(&time);
	strftime(str_t, 26, "%Y-%m-%d %H:%M:%S", p_time);
	stream_ << str_t;
	switch(level_) {
	case 0 :
		stream_ << "[UNKNOWN]";
		break;
	case 1 :
		stream_ << "[DEBUG]";
		break;
	case 2 :
		stream_ << "[INFO]";
		break;
	case 3 :
		stream_ << "[WARN]";
		break;
	case 4 :
		stream_ << "[ERROR]";
		break;
	case 5 :
		stream_ << "[FATAL]";
		break;
	}
}

Logger::Logger(const char* fileName, int line, int level) : impl_(fileName, line, level)
{
	char log_full_name[256] = {0};
	time_t t = time(NULL);
	struct tm *sys_tm = localtime(&t);
	struct tm my_tm = *sys_tm;
	char* file_name = "WebServerLog";
	snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
	setLogFileName(log_full_name);
}

Logger::~Logger() 
{
	impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';
	const LogStream::Buffer& buf(stream().buffer());
	output(buf.data(), buf.length());
}
