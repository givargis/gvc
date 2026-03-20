// Copyright (c) Tony Givargis, 2024-2026

#define _POSIX_C_SOURCE 200809L

#include <dlfcn.h>

#include "g_dl.h"

struct g_dl {
	void *handle;
};

g_dl_t
g_dl_open(const char *pathname)
{
	struct g_dl *dl;

	assert( pathname && (*pathname) );

	if (!(dl = g_malloc(sizeof (struct g_dl)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(dl, 0, sizeof (struct g_dl));
	if (!(dl->handle = dlopen(pathname, RTLD_LAZY | RTLD_LOCAL))) {
		g_dl_close(dl);
		G_TRACE("unable to load library");
		return NULL;
	}
	return dl;
}

void
g_dl_close(g_dl_t dl)
{
	if (dl) {
		if (dl->handle) {
			dlclose(dl->handle);
		}
		memset(dl, 0, sizeof (struct g_dl));
		g_free(dl);
	}
}

void *
g_dl_lookup(g_dl_t dl, const char *symbol)
{
	assert( dl && dl->handle );
	assert( symbol && (*symbol) );

	return dlsym(dl->handle, symbol);
}
