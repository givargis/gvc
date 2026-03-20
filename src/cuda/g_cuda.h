// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_CUDA_H_
#define _G_CUDA_H_

struct g_cuda_info {
	int size;
	struct g_cuda_info_device {
		const char *name;
		int hardware[3]; // processors * threads
		int threads[4];  // x * y * z <= max
		int blocks[3];   // x * y * z <= total
	} *devices;
};

int g_cuda_info_create(struct g_cuda_info *info);

void g_cuda_info_destroy(struct g_cuda_info *info);

#endif
