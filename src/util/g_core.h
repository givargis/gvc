// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_CORE_H_
#define _G_CORE_H_

#include <math.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#define G_MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#define G_MAX(a, b) ( ((a) > (b)) ? (a) : (b) )
#define G_DUP(a, b) ( (0 == ((a) % (b))) ? ((a) / (b)) : ((a) / (b) + 1) )

#define G_ARRAY_SIZE(a) ( sizeof ((a)) / sizeof ((a)[0]) )

#define G_TRACE(s)					\
	do {						\
		g_log("trace: %s:%d: %s",		\
		      __FILE__,				\
		      __LINE__,				\
		      (s) ? (s) : "");			\
	} while (0)

void g_init(int noinfo, int notrace, int noerror);

void g_log(const char *format, ...);

void g_sprintf(char *buf, size_t len, const char *format, ...);

void g_unlink(const char *pathname);

void g_usleep(unsigned us);

uint64_t g_time(void);

void *g_malloc(size_t n);

void *g_realloc(void *p, size_t n);

char *g_strdup(const char *s);

void g_free(void *p);

long g_lround(double x);

inline int
g_isnan(double x)
{
	return x != x;
}

inline int
g_isinf(double x)
{
	double diff = x - x;

	return (diff != diff) && (x == x);
}

inline unsigned
g_popcount(uint64_t x)
{
	return __builtin_popcountll(x);
}

inline unsigned
g_clz(uint64_t x)
{
	return x ? __builtin_clzll(x) : 64;
}

#endif
