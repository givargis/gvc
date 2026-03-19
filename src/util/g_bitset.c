// Copyright (c) Tony Givargis, 2024-2026

#include "g_bitset.h"

struct g_bitset {
	uint64_t ii;
	uint64_t size;
	uint64_t *bitmaps[2];
};

static void
set(struct g_bitset *bitset, uint64_t s, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	bitset->bitmaps[s][Q] |= ((uint64_t)1 << R);
}

static void
clr(struct g_bitset *bitset, uint64_t s, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	bitset->bitmaps[s][Q] &= ~((uint64_t)1 << R);
}

static int
get(const struct g_bitset *bitset, uint64_t s, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	if (i < bitset->size) {
		if ((bitset->bitmaps[s][Q] & ((uint64_t)1 << R))) {
			return 1;
		}
	}
	return 0;
}

g_bitset_t
g_bitset_open(uint64_t size)
{
	struct g_bitset *bitset;

	assert( size );

	if (!(bitset = g_malloc(sizeof (struct g_bitset)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(bitset, 0, sizeof (struct g_bitset));
	bitset->size = size;
	if (!(bitset->bitmaps[0] = g_malloc(G_DUP(bitset->size, 64) * 8)) ||
	    !(bitset->bitmaps[1] = g_malloc(G_DUP(bitset->size, 64) * 8))) {
		g_bitset_close(bitset);
		G_TRACE("^");
		return NULL;
	}
	memset(bitset->bitmaps[0], 0, G_DUP(bitset->size, 64) * 8);
	memset(bitset->bitmaps[1], 0, G_DUP(bitset->size, 64) * 8);
	set(bitset, 0, 0); // 0 is not a valid allocation
	return bitset;
}

void
g_bitset_close(g_bitset_t bitset)
{
	if (bitset) {
		g_free(bitset->bitmaps[0]);
		g_free(bitset->bitmaps[1]);
		memset(bitset, 0, sizeof (struct g_bitset));
		g_free(bitset);
	}
}

uint64_t
g_bitset_reserve(g_bitset_t bitset, uint64_t n)
{
	uint64_t k;

	assert( bitset && n );

	for (uint64_t j=0; j<bitset->size; ++j) {
		uint64_t i = (bitset->ii + j) % bitset->size;
		for (k=0; k<n; ++k) {
			if (((i+k) >= bitset->size) || get(bitset, 0, i + k)) {
				break;
			}
		}
		if (k == n) {
			for (k=0; k<n; ++k) {
				set(bitset, 0, i + k);
				set(bitset, 1, i + k);
			}
			clr(bitset, 1, i);
			bitset->ii = i + n;
			return i;
		}
	}
	return 0; // 0 is not a valid allocation
}

uint64_t
g_bitset_release(g_bitset_t bitset, uint64_t i)
{
	uint64_t n;

	assert( bitset && i );

	n = 0;
	if (get(bitset, 0, i) && !get(bitset, 1, i)) {
		do {
			clr(bitset, 0, i);
			clr(bitset, 1, i);
			++n;
			++i;
		}
		while (get(bitset, 1, i));
	}
	return n;
}

uint64_t
g_bitset_validate(g_bitset_t bitset, uint64_t i)
{
	uint64_t n;

	assert( bitset );

	n = 0;
	if (get(bitset, 0, i) && !get(bitset, 1, i)) {
		do {
			++n;
			++i;
		}
		while (get(bitset, 1, i));
	}
	return n;
}

uint64_t
g_bitset_utilized(g_bitset_t bitset)
{
	uint64_t n;

	assert( bitset );

	n = 0;
	for (uint64_t i=0; i<G_DUP(bitset->size, 64); ++i) {
		n += g_popcount(bitset->bitmaps[0][i]);
	}
	return n;
}

uint64_t
g_bitset_size(g_bitset_t bitset)
{
	assert( bitset );

	return bitset->size;
}
