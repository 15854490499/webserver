#ifndef __MACRO_H__
#define __MACRO_H__

#include <string.h>
#include <assert.h>
#include "log/log.h"

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#   define WEB_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#   define WEB_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define WEB_LIKELY(x)      (x)
#   define WEB_UNLIKELY(x)      (x)
#endif

#define WEB_ASSERT(x) \
	if(WEB_UNLIKELY(!(x))) { \
		LOG_ERROR("ASSERTION: "); \
		LOG_ERROR(x+"\n"); \
		assert(x); \
	}

#define WEB_ASSERT2(x, w) \
	if(WEB_UNLIKELY(!(x))) { \
		LOG_ERROR("ASSERTION\n"); \
		LOG_ERROR(w); \
		assert(x); \
	}

#endif
