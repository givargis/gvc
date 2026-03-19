// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_JITC_H_
#define _G_JITC_H_

#include "g_core.h"

typedef struct g_jitc *g_jitc_t;

int g_jitc_compile(const char *input, const char *output);

g_jitc_t g_jitc_open(const char *pathname);

void g_jitc_close(g_jitc_t jitc);

void *g_jitc_lookup(g_jitc_t jitc, const char *symbol);

#endif
