// Copyright (c) Tony Givargis, 2024-2026

#define _POSIX_C_SOURCE 200809L

#include <unistd.h>

#include "g_spinlock.h"
#include "g_core.h"

static int noinfo;
static int notrace;
static int noerror;
static int nocolor;

void
g_init(int noinfo_, int notrace_, int noerror_)
{
	noinfo = noinfo_ ? 1 : 0;
	notrace = notrace_ ? 1 : 0;
	noerror = noerror_ ? 1 : 0;
	nocolor = isatty(STDOUT_FILENO) ? 0 : 1;
	if ((1 != sizeof (char)) ||
	    (2 != sizeof (short)) ||
	    (4 != sizeof (int)) ||
	    (8 != sizeof (long)) ||
	    (4 != sizeof (float)) ||
	    (8 != sizeof (double)) ||
	    (8 != sizeof (void *)) ||
	    (8 != sizeof (size_t))) {
		G_TRACE("unsupported architecture (abort)");
		abort();
	}
}

void
g_log(const char *format, ...)
{
	static g_spinlock_t lock = G_SPINLOCK_INITIALIZER;
	struct tm tm;
	va_list ap;
	time_t t;
	enum {
		COLOR_,
		COLOR_INFO,
		COLOR_TRACE,
		COLOR_ERROR
	} mode = COLOR_;

	assert( format );

	t = time(NULL);
	localtime_r(&t, &tm);
	if (!strncmp(format, "info: ", 6)) {
		if (noinfo) {
			return;
		}
		mode = COLOR_INFO;
		format += 6;
	}
	else if (!strncmp(format, "trace: ", 7)) {
		if (notrace) {
			return;
		}
		mode = COLOR_TRACE;
		format += 7;
	}
	else if (!strncmp(format, "error: ", 7)) {
		if (noerror) {
			return;
		}
		mode = COLOR_ERROR;
		format += 7;
	}
	else {
		assert( 0 );
		return;
	}
	g_spinlock_lock(&lock);

	// M.E. -->

	printf("%s" "[%02d/%02d/%d %02d:%02d:%02d] " "%s",
	       nocolor ? "" : "\033[36m", // cyan
	       tm.tm_mon + 1,
	       tm.tm_mday,
	       tm.tm_year + 1900,
	       tm.tm_hour,
	       tm.tm_min,
	       tm.tm_sec,
	       nocolor ? "" : "\033[0m");
	if (COLOR_INFO == mode) {
		printf("%s" "info:  " "%s",
		       nocolor ? "" : "\033[1;32m", // bold-green
		       nocolor ? "" : "\033[0m");
	}
	else if (COLOR_TRACE == mode) {
		printf("%s" "trace: " "%s",
		       nocolor ? "" : "\033[1;34m", // bold-blue
		       nocolor ? "" : "\033[0m");
	}
	else if (COLOR_ERROR == mode) {
		printf("%s" "error: " "%s",
		       nocolor ? "" : "\033[1;31m", // bold-red
		       nocolor ? "" : "\033[0m");
	}
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	printf("\n");

	// M.E. <--

	g_spinlock_unlock(&lock);
}

void
g_sprintf(char *buf, size_t len, const char *format, ...)
{
	va_list ap;
	int rv;

	assert( (!len || buf) && format );

	va_start(ap, format);
	rv = vsnprintf(buf, len, format, ap);
	va_end(ap);
	if ((0 > rv) || (len <= (size_t)rv)) {
		G_TRACE("software error (abort)");
		abort();
	}
}

void
g_unlink(const char *pathname)
{
	if (pathname && (*pathname)) {
		remove(pathname);
	}
}

void
g_usleep(unsigned us)
{
	struct timespec in, out;

	in.tv_sec = (time_t)(us / 1000000);
	in.tv_nsec = (long)(us % 1000000) * 1000;
	while ((-1 == nanosleep(&in, &out)) && (EINTR == errno)) {
		in = out;
	}
}

uint64_t
g_time(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
		G_TRACE("unable to get kernel time (abort)");
		abort();
	}
	return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

void *
g_malloc(size_t n)
{
	void *p;

	assert( n );

	if (!(p = malloc(n))) {
		G_TRACE("out of memory");
		return NULL;
	}
	return p;
}

void *
g_realloc(void *p_, size_t n)
{
	void *p;

	assert( n );

	if (!(p = realloc(p_, n))) {
		G_TRACE("out of memory");
		return NULL;
	}
	return p;
}

char *
g_strdup(const char *s)
{
	size_t n;
	char *p;

	s = s ? s : "";
	n = strlen(s);
	if (!(p = malloc(n + 1))) {
		G_TRACE("^");
		return NULL;
	}
	memcpy(p, s, n);
	p[n] = '\0';
	return p;
}

void
g_free(void *p)
{
	free(p);
}

long
g_lround(double x)
{
	if (x >= 0.0) {
		return (long)floor(x + 0.5);
	}
	return (long)ceil(x - 0.5);
}
