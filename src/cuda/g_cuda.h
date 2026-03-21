// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_CUDA_H_
#define _G_CUDA_H_

#include "../util/g_util.h"

struct g_cuda_info {
	int size;
	struct g_cuda_info_device {
		struct {
			int major;
			int minor;
			const char *name;
		} id;
		struct {
			int warp;
			int threads;
			int processors;
		} compute;
		struct {
			int blocks[3];   // x * y * z
			int threads[4];  // x * y * z (max)
		} geometry;
	} *devices;
};

int g_cuda_info_create(struct g_cuda_info *info);

void g_cuda_info_destroy(struct g_cuda_info *info);

void g_cuda_info_print(const struct g_cuda_info *info);

int g_cuda_execute(void);

#endif
