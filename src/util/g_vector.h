// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_VECTOR_H_
#define _G_VECTOR_H_

#include "g_core.h"

typedef struct g_vector *g_vector_t;

g_vector_t g_vector_open(void);

void g_vector_close(g_vector_t vector);

void *g_vector_append(g_vector_t vector, size_t n);

void *g_vector_lookup(g_vector_t vector, uint64_t i);

uint64_t g_vector_items(g_vector_t vector);

#endif
