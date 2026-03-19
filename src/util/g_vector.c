// Copyright (c) Tony Givargis, 2024-2026

#include "g_vector.h"

struct g_vector {
	void **mem;
	uint64_t size;
	uint64_t items;
};

g_vector_t
g_vector_open(void)
{
	struct g_vector *vector;

	if (!(vector = g_malloc(sizeof (struct g_vector)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(vector, 0, sizeof (struct g_vector));
	return vector;
}

void
g_vector_close(g_vector_t vector)
{
	if (vector) {
		if (vector->mem) {
			for (uint64_t i=0; i<vector->items; ++i) {
				g_free(vector->mem[i]);
			}
		}
		g_free(vector->mem);
		memset(vector, 0, sizeof (struct g_vector));
		g_free(vector);
	}
}

void *
g_vector_append(g_vector_t vector, size_t n)
{
	const size_t INCREMENT = 100;
	void **mem;

	assert( vector && (0 < n) );

	if (vector->items >= vector->size) {
		uint64_t size = vector->size + INCREMENT;
		size_t m = size * sizeof (vector->mem[0]);
		if (!(mem = g_realloc(vector->mem, m))) {
			G_TRACE("^");
			return NULL;
		}
		memset(mem + vector->size,
		       0,
		       INCREMENT * sizeof (vector->mem[0]));
		vector->mem = mem;
		vector->size = size;
	}
	if (!(vector->mem[vector->items] = g_malloc(n))) {
		G_TRACE("^");
		return NULL;
	}
	memset(vector->mem[vector->items], 0, n);
	return vector->mem[vector->items++];
}

void *
g_vector_lookup(g_vector_t vector, uint64_t i)
{
	assert( vector && (vector->items > i) );

	return vector->mem[i];
}

uint64_t
g_vector_items(g_vector_t vector)
{
	assert( vector );

	return vector->items;
}
