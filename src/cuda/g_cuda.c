// Copyright (c) Tony Givargis, 2024-2026

#include <cuda.h>
#include <cuda_runtime.h>

#include "../util/g_util.h"

int
g_cuda_enumerate(void)
{
	struct cudaDeviceProp prop;
	cudaError_t ce;
	int n;

	ce = cudaGetDeviceCount(&n);
	if (cudaSuccess != ce) {
		G_TRACE("cudaGetDeviceCount() failed");
		return -1;
	}
	for (int i=0; i<n; ++i) {
		ce = cudaGetDeviceProperties(&prop, i);
		if (cudaSuccess != ce) {
			G_TRACE("cudaGetDeviceProperties() failed");
			return -1;
		}
		g_log("info: --- %s (id:%d) ---", prop.name, i);
		g_log("info: asyncEngineCount   : %d", prop.asyncEngineCount);
		g_log("info: concurrentKernels  : %d", prop.concurrentKernels);
		g_log("info: maxThreadsPerBlock : %d",
		      prop.maxThreadsPerBlock);
		g_log("info: multiProcessorCount: %d",
		      prop.multiProcessorCount);
		g_log("info: maxThreadsDim[]    : %d %d %d",
		      prop.maxThreadsDim[0],
		      prop.maxThreadsDim[1],
		      prop.maxThreadsDim[2]);
		g_log("info: maxGridSize[]      : %d %d %d",
		      prop.maxGridSize[0],
		      prop.maxGridSize[1],
		      prop.maxGridSize[2]);
	}
	return 0;
}
