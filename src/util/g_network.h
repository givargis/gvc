// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_NETWORK_H_
#define _G_NETWORK_H_

#include "g_core.h"

typedef struct g_network *g_network_t;

typedef void (*g_network_fnc_t)(void *ctx, g_network_t network);

g_network_t g_network_server(const char *hostname,
			     const char *servname,
			     g_network_fnc_t fnc,
			     void *ctx);

g_network_t g_network_connect(const char *hostname, const char *servname);

void g_network_close(g_network_t network);

int g_network_read(g_network_t network, void *buf, size_t len);

int g_network_write(g_network_t network, const void *buf, size_t len);

int g_network_is_valid(const char *address);

#endif
