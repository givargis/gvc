// Copyright (c) Tony Givargis, 2024-2026

#include "g_hash.h"

static uint64_t
shuffle(uint64_t *x, uint64_t *y, uint64_t z)
{
	(*x) += z;
	(*x)  = ((*x) << 30) | ((*x) >> (64 - 30));
	(*x) *= 14400146411488415129U;
	(*x)  = ((*x) << 14) | ((*x) >> (64 - 14));
	(*y) ^= z;
	(*y)  = ((*y) << 34) | ((*y) >> (64 - 34));
	(*y) *= 10911097110311091151U;
	(*y)  = ((*y) << 27) | ((*y) >> (64 - 27));
	return (*x) ^ (*y);
}

uint64_t
g_hash(const void *buf_, size_t len)
{
	const char *buf = (const char *)buf_;
	uint64_t x, y, z;
	size_t q, r;

	assert( !len || buf );

	q = len / 8;
	r = len % 8;
	x = 13464654573299691533U;
	y = 10141516181932277123U;
	for (size_t i=0; i<q; ++i) {
		memcpy(&z, buf, 8);
		shuffle(&x, &y, z);
		buf += 8;
	}
	if (r) {
		z = 0;
		memcpy(&z, buf, r);
		shuffle(&x, &y, z);
	}
	shuffle(&x, &y, (uint64_t)len);
	return x + y;
}
