#pragma once

#include <string.h>
#include <assert.h>
#include "log.h"

#if defined __GNUC__ || defined __llvm__
#	define LIKELY(x) __builtin_expect(!!(x), 1)
#	define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#	define LIKELY(x) (x)
#	define UNLIKELY(x) (x)
#endif


#define LOG_ASSERT(x) \
	if(UNLIKELY(!(x))) { \
		LOG_INFO << "ASSEERTION: " #x; \
		assert(x); \
	}

#define LOG_ASSERT2(x, w) \
	if(UNLIKELY(!(x))) { \
		LOG_INFO << "ASSERTION: " #x \
			<< "\n" << w; \
		assert(x); \
	}


