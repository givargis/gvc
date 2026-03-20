// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_DL_H_
#define _G_DL_H_

#include "g_core.h"

#define G_DL_FNC(f) ( (*(void **)&f) )

typedef struct g_dl *g_dl_t;

void g_dl_init(void);

g_dl_t g_dl_open(const char *pathname);

void g_dl_close(g_dl_t dl);

void *g_dl_lookup(g_dl_t dl, const char *symbol);

#endif
