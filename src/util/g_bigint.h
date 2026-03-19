// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_BIGINT_H_
#define _G_BIGINT_H_

#include "g_core.h"

typedef struct g_bigint *g_bigint_t;

extern const g_bigint_t G_BIGINT_CONST[17]; // 0 - 16

void g_bigint_free(g_bigint_t z);

g_bigint_t g_bigint_int(int64_t v);

g_bigint_t g_bigint_real(double r);

g_bigint_t g_bigint_string(const char *s);

char *g_bigint_print(g_bigint_t a); // caller free

g_bigint_t g_bigint_add(g_bigint_t a, g_bigint_t b);

g_bigint_t g_bigint_sub(g_bigint_t a, g_bigint_t b);

g_bigint_t g_bigint_mul(g_bigint_t a, g_bigint_t b);

g_bigint_t g_bigint_div(g_bigint_t a, g_bigint_t b);

g_bigint_t g_bigint_mod(g_bigint_t a, g_bigint_t b);

int g_bigint_divmod(g_bigint_t a,
		    g_bigint_t b,
		    g_bigint_t *r,
		    g_bigint_t *q);

int g_bigint_cmp(g_bigint_t a, g_bigint_t b);

int g_bigint_bits(g_bigint_t a);

int g_bigint_digits(g_bigint_t a);

int g_bigint_is_zero(g_bigint_t a);

int g_bigint_is_one(g_bigint_t a);

int g_bigint_is_negative(g_bigint_t a);

#endif
