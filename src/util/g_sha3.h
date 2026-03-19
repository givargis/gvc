// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_SHA3_H_
#define _G_SHA3_H_

#include "g_core.h"

#define G_SHA3_LEN 32

void g_sha3(const void *buf, size_t len, void *out);

#endif
