// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_MAP_H_
#define _G_MAP_H_

#include "g_core.h"

typedef struct g_map *g_map_t;

typedef int (*g_map_fnc_t)(void *ctx, const char *key, void *val);

g_map_t g_map_open(void);

void g_map_close(g_map_t map);

void g_map_empty(g_map_t map);

int g_map_update(g_map_t map, const char *key, const void *val);

void *g_map_lookup(g_map_t map, const char *key);

int g_map_iterate(g_map_t map, g_map_fnc_t fnc, void *ctx);

uint64_t g_map_items(g_map_t map);

#endif
