// Copyright (c) Tony Givargis, 2024-2026

#include <cuda.h>
#include <cuda_runtime.h>

#include "../util/g_util.h"

#include "g_cuda.h"

int
g_cuda_info_create(struct g_cuda_info *info)
{
	struct g_cuda_info_device *device;
	struct cudaDeviceProp prop;

	assert( info );

	memset(info, 0, sizeof (struct g_cuda_info));
	if (cudaSuccess != cudaGetDeviceCount(&info->size)) {
		g_cuda_info_destroy(info);
		G_TRACE("cudaGetDeviceCount() failed");
		return -1;
	}
	if (!info->size) {
		return 0;
	}
	info->devices = g_malloc(info->size * sizeof (info->devices[0]));
	if (!info->devices) {
		g_cuda_info_destroy(info);
		G_TRACE("^");
		return -1;
	}
	memset(info->devices, 0, info->size * sizeof (info->devices[0]));
	for (int i=0; i<info->size; ++i) {
		device = &info->devices[i];
		if (cudaSuccess != cudaGetDeviceProperties(&prop, i)) {
			g_cuda_info_destroy(info);
			G_TRACE("cudaGetDeviceProperties() failed");
			return -1;
		}
		if (!(device->name = g_strdup(prop.name))) {
			g_cuda_info_destroy(info);
			G_TRACE("^");
			return -1;
		}
		device->hardware[0] = prop.multiProcessorCount;
		device->hardware[1] = prop.maxThreadsPerMultiProcessor;
		device->threads[0] = prop.maxThreadsDim[0];
		device->threads[1] = prop.maxThreadsDim[1];
		device->threads[2] = prop.maxThreadsDim[2];
		device->threads[3] = prop.maxThreadsPerBlock;
		device->blocks[0] = prop.maxGridSize[0];
		device->blocks[1] = prop.maxGridSize[1];
		device->blocks[2] = prop.maxGridSize[2];
	}
	return 0;
}

void
g_cuda_info_destroy(struct g_cuda_info *info)
{
	assert( info );

	if (info->devices) {
		for (int i=0; i<info->size; ++i) {
			g_free((void *)info->devices[i].name);
		}
		g_free(info->devices);
	}
	memset(info, 0, sizeof (struct g_cuda_info));
}
