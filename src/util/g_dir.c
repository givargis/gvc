// Copyright (c) Tony Givargis, 2024-2026

#define _POSIX_C_SOURCE 200809L

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "g_dir.h"

static int
search(const char *pathname, g_dir_fnc_t fnc, void *ctx)
{
	struct dirent *dirent;
	struct stat stat;
	char *pathname_;
	DIR *dir;

	if (lstat(pathname, &stat)) {
		G_TRACE("unable to stat file");
		return -1;
	}
	if (!S_ISDIR(stat.st_mode)) {
		if (S_ISREG(stat.st_mode)) {
			if (fnc(ctx, pathname)) {
				G_TRACE("^");
				return -1;
			}
		}
		return 0;
	}
	if (!(dir = opendir(pathname))) {
		G_TRACE("unable to open directory");
		return -1;
	}
	while ((dirent = readdir(dir))) {
		if (!strcmp(dirent->d_name, ".") ||
		    !strcmp(dirent->d_name, "..")) {
			continue;
		}
		size_t n = strlen(pathname) + strlen(dirent->d_name) + 2;
		if (!(pathname_ = g_malloc(n))) {
			closedir(dir);
			G_TRACE("^");
			return -1;
		}
		g_sprintf(pathname_, n, "%s/%s", pathname, dirent->d_name);
		if (search(pathname_, fnc, ctx)) {
			closedir(dir);
			g_free(pathname_);
			G_TRACE("^");
			return -1;
		}
		g_free(pathname_);
	}
	closedir(dir);
	return 0;
}

int
g_dir(const char *pathname, g_dir_fnc_t fnc, void *ctx)
{
	assert( pathname && (*pathname) && fnc );

	if (search(pathname, fnc, ctx)) {
		G_TRACE("^");
		return -1;
	}
	return 0;
}
