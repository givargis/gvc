// Copyright (c) Tony Givargis, 2024-2026

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>

#include "g_jitc.h"

struct g_jitc {
	void *handle;
};

int
g_jitc_compile(const char *input, const char *output)
{
	const char *argv[] = {
		"/usr/bin/gcc",
		"-O3",
		"-fpic",
		"-shared",
		"-o",
		NULL, // output
		NULL, // input
		NULL
	};
	pid_t pid, pid_;
	int status;

	assert( input && (*input) );
	assert( output && (*output) );

	argv[G_ARRAY_SIZE(argv) - 2] = input;
	argv[G_ARRAY_SIZE(argv) - 3] = output;
	if (0 > (pid = fork())) {
		G_TRACE("unable to fork child process");
		return -1;
	}
	else if (!pid) {
		execv(argv[0], (char * const *)argv);
		G_TRACE("unable to execute child process (abort)");
		abort();
		return -1;
	}
	else {
		for (;;) {
			pid_ = waitpid(pid, &status, 0);
			if ((-1 == pid_) && (EINTR == errno)) {
				continue;
			}
			break;
		}
		if ((-1 == pid_) || (pid_ != pid) || !WIFEXITED(status)) {
			G_TRACE("system failure detected (abort)");
			abort();
			return -1;
		}
		if (WEXITSTATUS(status)) {
			G_TRACE("system failure detected");
			return -1;
		}
	}
	return 0;
}

g_jitc_t
g_jitc_open(const char *pathname)
{
	struct g_jitc *jitc;

	assert( pathname && (*pathname) );

	if (!(jitc = g_malloc(sizeof (struct g_jitc)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(jitc, 0, sizeof (struct g_jitc));
	if (!(jitc->handle = dlopen(pathname, RTLD_LAZY | RTLD_LOCAL))) {
		g_jitc_close(jitc);
		G_TRACE("unable to load library");
		return NULL;
	}
	return jitc;
}

void
g_jitc_close(g_jitc_t jitc)
{
	if (jitc) {
		if (jitc->handle) {
			dlclose(jitc->handle);
		}
		memset(jitc, 0, sizeof (struct g_jitc));
		g_free(jitc);
	}
}

void *
g_jitc_lookup(g_jitc_t jitc, const char *symbol)
{
	assert( jitc && jitc->handle );
	assert( symbol && (*symbol) );

	return dlsym(jitc->handle, symbol);
}
