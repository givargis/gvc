// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_FFT_H_
#define _G_FFT_H_

#include "g_core.h"

struct g_fft_complex {
	float r;
	float i;
};

void g_fft_forward(struct g_fft_complex *signal, int n);

void g_fft_inverse(struct g_fft_complex *signal, int n);

#endif
