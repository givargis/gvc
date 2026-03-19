// Copyright (c) Tony Givargis, 2024-2026

#define _POSIX_C_SOURCE 200809L

#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "g_file.h"

struct g_file {
	int fd;
};

g_file_t
g_file_open(const char *pathname, int mode_)
{
	struct g_file *file;
	int mode;

	assert( pathname && (*pathname) );

	mode  = (G_FILE_MODE_RDWR & mode_) ? O_RDWR : O_RDONLY;
	mode |= (G_FILE_MODE_CREATE & mode_) ? O_CREAT : 0;
	mode |= (G_FILE_MODE_TRUNCATE & mode_) ? O_TRUNC : 0;
	if (!(file = g_malloc(sizeof (struct g_file)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(file, 0, sizeof (struct g_file));
	file->fd = -1;
	if (0 > (file->fd = open(pathname, mode, 0600))) {
		g_file_close(file);
		G_TRACE("unable to open file");
		return NULL;
	}
	return file;
}

void
g_file_close(g_file_t file)
{
	if (file) {
		if (0 <= file->fd) {
			close(file->fd);
		}
		memset(file, 0, sizeof (struct g_file));
		g_free(file);
	}
}

int
g_file_sync(g_file_t file)
{
	assert( file && (0 <= file->fd) );

	if (fsync(file->fd)) {
		G_TRACE("unable to sync file");
		return -1;
	}
	return 0;
}

int
g_file_read(g_file_t file, void *buf_, size_t off, size_t len)
{
	char *buf = (char *)buf_;

	assert( file && (0 <= file->fd) && (!len || buf) );

	while (len) {
		ssize_t n = pread(file->fd, buf, len, (off_t)off);
		if (!n) {
			G_TRACE("unexpected EOF while reading file");
			return -1;
		}
		if (0 > n) {
			if (EINTR != errno) {
				G_TRACE("unable to read file");
				return -1;
			}
			continue;
		}
		buf += (size_t)n;
		off += (size_t)n;
		len -= (size_t)n;
	}
	return 0;
}

int
g_file_write(g_file_t file, const void *buf_, size_t off, size_t len)
{
	const char *buf = (const char *)buf_;

	assert( file && (0 <= file->fd) && (!len || buf) );

	while (len) {
		ssize_t n = pwrite(file->fd, buf, len, (off_t)off);
		if (!n) {
			G_TRACE("unexpected EOF while writing file");
			return -1;
		}
		if (0 > n) {
			if (EINTR != errno) {
				G_TRACE("unable to write file");
				return -1;
			}
			continue;
		}
		buf += (size_t)n;
		off += (size_t)n;
		len -= (size_t)n;
	}
	return 0;
}

int
g_file_size(g_file_t file, size_t *size)
{
	struct stat st;

	assert( file && (0 <= file->fd) && size );

	if (fstat(file->fd, &st)) {
		G_TRACE("unable to stat file");
		return -1;
	}
	if (!S_ISREG(st.st_mode)) {
		G_TRACE("not a regular file");
		return -1;
	}
	(*size) = (size_t)st.st_size;
	return 0;
}

char *
g_file_string_read(const char *pathname)
{
	const size_t PAD = 8;
	g_file_t file;
	size_t n;
	char *s;

	assert( pathname && (*pathname) );

	if (!(file = g_file_open(pathname, G_FILE_MODE_RD))) {
		G_TRACE("^");
		return NULL;
	}
	if (g_file_size(file, &n)) {
		g_file_close(file);
		G_TRACE("^");
		return NULL;
	}
	if (!(s = g_malloc(n + PAD))) {
		g_file_close(file);
		G_TRACE("^");
		return NULL;
	}
	if (g_file_read(file, s, 0, n)) {
		g_free(s);
		g_file_close(file);
		G_TRACE("^");
		return NULL;
	}
	g_file_close(file);
	memset(s + n, '\0', PAD);
	return s;
}

int
g_file_string_write(const char *pathname, const char *s)
{
	g_file_t file;

	assert( pathname && (*pathname) );

	if (!(file = g_file_open(pathname,
				 G_FILE_MODE_RDWR |
				 G_FILE_MODE_CREATE |
				 G_FILE_MODE_TRUNCATE)) ||
	    g_file_write(file, s, 0, s ? strlen(s) : 0)) {
		g_file_close(file);
		G_TRACE("^");
		return -1;
	}
	g_file_close(file);
	return 0;
}
