// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_DIR_H_
#define _G_DIR_H_

#include "g_core.h"

typedef int (*g_dir_fnc_t)(void *ctx, const char *pathname);

int g_dir(const char *pathname, g_dir_fnc_t fnc, void *ctx);

#endif
