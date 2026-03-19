// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_FILE_H_
#define _G_FILE_H_

#include "g_core.h"

#define G_FILE_MODE_RD       0
#define G_FILE_MODE_RDWR     1
#define G_FILE_MODE_CREATE   2
#define G_FILE_MODE_TRUNCATE 4

typedef struct g_file *g_file_t;

g_file_t g_file_open(const char *pathname, int mode);

void g_file_close(g_file_t file);

int g_file_sync(g_file_t file);

int g_file_read(g_file_t file, void *buf, size_t off, size_t len);

int g_file_write(g_file_t file, const void *buf, size_t off, size_t len);

int g_file_size(g_file_t file, size_t *size);

char *g_file_string_read(const char *pathname); // caller free

int g_file_string_write(const char *pathname, const char *s);

#endif
