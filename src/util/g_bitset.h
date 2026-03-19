// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_BITSET_H_
#define _G_BITSET_H_

#include "g_core.h"

typedef struct g_bitset *g_bitset_t;

g_bitset_t g_bitset_open(uint64_t size);

void g_bitset_close(g_bitset_t bitset);

uint64_t g_bitset_reserve(g_bitset_t bitset, uint64_t n);

uint64_t g_bitset_release(g_bitset_t bitset, uint64_t i);

uint64_t g_bitset_validate(g_bitset_t bitset, uint64_t i);

uint64_t g_bitset_utilized(g_bitset_t bitset);

uint64_t g_bitset_size(g_bitset_t bitset);

#endif
