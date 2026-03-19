// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_THREAD_H_
#define _G_THREAD_H_

#include "g_core.h"

typedef struct g_thread *g_thread_t;
typedef struct g_mutex *g_mutex_t;
typedef struct g_cond *g_cond_t;

typedef void (*g_thread_fnc_t)(void *ctx);

g_thread_t g_thread_open(g_thread_fnc_t fnc, void *ctx);

void g_thread_close(g_thread_t thread);

int g_thread_good(g_thread_t thread);

g_mutex_t g_mutex_open(void);

void g_mutex_close(g_mutex_t mutex);

void g_mutex_lock(g_mutex_t mutex);

void g_mutex_unlock(g_mutex_t mutex);

g_cond_t g_cond_open(g_mutex_t mutex);

void g_cond_close(g_cond_t cond);

void g_cond_signal(g_cond_t cond);

void g_cond_wait(g_cond_t cond);

#endif
