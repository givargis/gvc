// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_BASE64_H_
#define _G_BASE64_H_

#include "g_core.h"

#define G_BASE64_ENCODE_LEN(n) ( ((((n) + 2) / 3) * 4) + 1 )
#define G_BASE64_DECODE_LEN(n) ( ((((n) + 3) / 4) * 3) + 0 )

int g_base64_encode(const void *buf, size_t len, char *s);

int g_base64_decode(void *buf, size_t *len, const char *s);

#endif
