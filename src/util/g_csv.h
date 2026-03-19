// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_CSV_H_
#define _G_CSV_H_

#include "g_core.h"

typedef struct g_csv *g_csv_t;

g_csv_t g_csv_open(const char *s);

void g_csv_close(g_csv_t csv);

const char *g_csv_cell(g_csv_t csv, uint64_t row, uint64_t col);

uint64_t g_csv_rows(g_csv_t csv);

uint64_t g_csv_cols(g_csv_t csv);

#endif
