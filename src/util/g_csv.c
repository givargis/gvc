// Copyright (c) Tony Givargis, 2024-2026

#include "g_csv.h"

struct g_csv {
	char *buf;
	uint64_t n_rows;
	uint64_t n_cols;
	const char **cells;
};

static void
populate(struct g_csv *csv, uint64_t r, uint64_t c, char *s)
{
	if (csv->cells) {
		while ('\n' == (*s)) {
			++s;
		}
		if (('"' == s[0]) && ('"' == s[strlen(s) - 1])) {
			++s;
			s[strlen(s) - 1] = '\0';
		}
		csv->cells[r * csv->n_cols + c] = s;
	}
}

static void
parse(struct g_csv *csv)
{
	int quote, field;
	uint64_t j, c, r;
	size_t i, n;
	char *p;

	p = csv->buf;
	j = c = r = 0;
	quote = field = 0;
	n = strlen(csv->buf);
	for (i=0; i<n; ++i) {
		if (csv->buf[i] == '"') {
			if (quote && ('"' == csv->buf[i + 1])) {
				++i;
			}
			else {
				quote = !quote;
				field = 1;
			}
		}
		else if (!quote) {
			if (',' == csv->buf[i]) {
				if (csv->cells) {
					csv->buf[i] = '\0';
					populate(csv, r, j, p);
					p = &csv->buf[i + 1];
				}
				++j;
				field = 0;
			}
			else if ('\n' == csv->buf[i]) {
				if (field || j) {
					if (csv->cells) {
						csv->buf[i] = '\0';
						populate(csv, r, j, p);
						p = &csv->buf[i + 1];
					}
					++j;
				}
				++r;
				c = G_MAX(c, j);
				j = 0;
				field = 0;
			}
			else if ('\r' == csv->buf[i]) {
				// ignore
			}
			else {
				field = 1;
			}
		}
		else {
			field = 1;
		}
	}
	if (field || j) {
		if (csv->cells) {
			csv->buf[i] = '\0';
			populate(csv, r, j, p);
		}
		++j;
		++r;
		c = G_MAX(c, j);
	}
	csv->n_rows = r;
	csv->n_cols = c;
}

g_csv_t
g_csv_open(const char *s)
{
	struct g_csv *csv;
	uint64_t n;

	assert( s );

	if (!(csv = g_malloc(sizeof (struct g_csv)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(csv, 0, sizeof (struct g_csv));
	if (!(csv->buf = g_strdup(s))) {
		g_csv_close(csv);
		G_TRACE("^");
		return NULL;
	}
	parse(csv);
	if (csv->n_cols && csv->n_rows) {
		n = csv->n_rows * csv->n_cols * sizeof (csv->cells[0]);
		if (!(csv->cells = g_malloc(n))) {
			g_csv_close(csv);
			G_TRACE("^");
			return NULL;
		}
		memset(csv->cells, 0, n);
		parse(csv);
	}
	else {
		csv->n_cols = 0;
		csv->n_rows = 0;
	}
	return csv;
}

void
g_csv_close(g_csv_t csv)
{
	if (csv) {
		g_free(csv->cells);
		g_free(csv->buf);
		memset(csv, 0, sizeof (struct g_csv));
		g_free(csv);
	}
}

const char *
g_csv_cell(g_csv_t csv, uint64_t row, uint64_t col)
{
	assert( csv );
	assert( row < csv->n_rows );
	assert( col < csv->n_cols );

	return csv->cells[row * csv->n_cols + col];
}

uint64_t
g_csv_rows(g_csv_t csv)
{
	assert( csv );

	return csv->n_rows;
}

uint64_t
g_csv_cols(g_csv_t csv)
{
	assert( csv );

	return csv->n_cols;
}
