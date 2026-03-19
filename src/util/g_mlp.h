// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_MLP_H_
#define _G_MLP_H_

#include "g_mlp.h"

typedef struct g_mlp *g_mlp_t;

g_mlp_t g_mlp_open(int input, int output, int hidden, int layers);

void g_mlp_close(g_mlp_t mlp);

const float *g_mlp_activate(g_mlp_t mlp, const float *x);

void g_mlp_train(g_mlp_t mlp,
		 const float *x,
		 const float *y,
		 float learning_rate,
		 int batch_size);

int g_mlp_load(g_mlp_t mlp, const char *pathname);

int g_mlp_store(g_mlp_t mlp, const char *pathname);

#endif
