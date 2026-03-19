// Copyright (c) Tony Givargis, 2024-2026

#include "g_random.h"

uint64_t
g_random(void)
{
	static uint64_t x, y;

	x += 13464654573299691533U;
	x  = (x << 30) | (x >> (64 - 30));
	x *= 14400146411488415129U;
	x  = (x << 14) | (x >> (64 - 14));
	y += 10141516181932277123U;
	y  = (y << 34) | (y >> (64 - 34));
	y *= 10911097110311091151U;
	y  = (y << 27) | (y >> (64 - 27));
	return x ^ y;
}
