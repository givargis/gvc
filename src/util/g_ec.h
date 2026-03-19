// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_EC_H_
#define _G_EC_H_

#include "g_core.h"

#define G_EC_MIN_K 1
#define G_EC_MAX_K 255

#define G_EC_MIN_N 8
#define G_EC_MAX_N 262144 // 8^0, 8^1 ... 8^6

#define G_EC_D(buf, j, n) ( (uint64_t *)((char *)(buf) + ((j) + 0) * (n)) )
#define G_EC_P(buf, k, n) ( (uint64_t *)((char *)(buf) + ((k) + 0) * (n)) )
#define G_EC_Q(buf, k, n) ( (uint64_t *)((char *)(buf) + ((k) + 1) * (n)) )

/**
 *  <---------- buf --------->
 *  <- n -><- n -> ... <- n ->
 *     0      1    ...    k
 */

void g_ec_init(void);

void g_ec_encode_pq(void *buf, int k, int n);

void g_ec_encode_p(void *buf, int k, int n);

void g_ec_encode_q(void *buf, int k, int n);

void g_ec_encode_dp(void *buf, int k, int n, int x);

void g_ec_encode_dq(void *buf, int k, int n, int x);

void g_ec_encode_dd(void *buf, int k, int n, int x, int y);

#endif
