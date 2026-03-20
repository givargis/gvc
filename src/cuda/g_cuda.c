// Copyright (c) Tony Givargis, 2024-2026

#include <cuda.h>
#include <cuda_runtime.h>

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

		// id

		if (!(device->id.name = g_strdup(prop.name))) {
			g_cuda_info_destroy(info);
			G_TRACE("^");
			return -1;
		}
		device->id.major = prop.major;
		device->id.minor = prop.minor;

		// compute

		device->compute.warp = prop.warpSize;
		device->compute.threads = prop.maxThreadsPerMultiProcessor;
		device->compute.processors = prop.multiProcessorCount;

		// geometry

		device->geometry.blocks[0] = prop.maxGridSize[0];
		device->geometry.blocks[1] = prop.maxGridSize[1];
		device->geometry.blocks[2] = prop.maxGridSize[2];
		device->geometry.threads[0] = prop.maxThreadsDim[0];
		device->geometry.threads[1] = prop.maxThreadsDim[1];
		device->geometry.threads[2] = prop.maxThreadsDim[2];
		device->geometry.threads[3] = prop.maxThreadsPerBlock;
	}
	return 0;
}

void
g_cuda_info_destroy(struct g_cuda_info *info)
{
	assert( info );

	if (info->devices) {
		for (int i=0; i<info->size; ++i) {
			g_free((void *)info->devices[i].id.name);
		}
		g_free(info->devices);
	}
	memset(info, 0, sizeof (struct g_cuda_info));
}

void
g_cuda_info_print(const struct g_cuda_info *info)
{
	const struct g_cuda_info_device *device;

	assert( info );

	for (int i=0; i<info->size; ++i) {
		device = &info->devices[i];
		g_log("info: %s %d.%d {",
		      device->id.name,
		      device->id.major,
		      device->id.minor);
		g_log("info:   compute {");
		g_log("info:     %d * %d (warp: %d)",
		      device->compute.threads,
		      device->compute.processors,
		      device->compute.warp);
		g_log("info:   }");
		g_log("info:   geometry {");
		g_log("info:     blocks {");
		g_log("info:       %d * %d * %d",
		      device->geometry.blocks[0],
		      device->geometry.blocks[1],
		      device->geometry.blocks[2]);
		g_log("info:     }");
		g_log("info:     threads {");
		g_log("info:       %d * %d * %d (max: %d)",
		      device->geometry.threads[0],
		      device->geometry.threads[1],
		      device->geometry.threads[2],
		      device->geometry.threads[3]);
		g_log("info:     }");
		g_log("info:   }");
		g_log("info: }");
	}
}

typedef void (*add_fnc_t)(float *a, float *b, float *c, int n);

#define N 50

void
stage(void)
{
	const char *argv[] = {
		"/usr/bin/gcc",
		"-O3",
		"-fpic",
		"-shared",
		"-o",
		NULL, // output
		NULL, // input
		NULL
	};
       pid_t pid, pid_;
       int status;

       assert( input && (*input) );
       assert( output && (*output) );

       argv[G_ARRAY_SIZE(argv) - 2] = input;
       argv[G_ARRAY_SIZE(argv) - 3] = output;

	g_dl_t dl;
	add_fnc_t add;
	float a[N];
	float b[N];
	float c[N];

	for (int i=0; i<N; ++i) {
		a[i] = g_random() / (double)~0LLU;
		b[i] = g_random() / (double)~0LLU;
		c[i] = 0.0;
	}

	g_execute(argv);
	return 0;

	dl = g_dl_open("./libtest.so");

	G_DL_FNC(add) = g_dl_lookup(dl, "vector_add");

	add(a, b, c, N);

	for (int i=0; i<N; ++i) {
		printf("%f + %f = %f\n", a[i], b[i], c[i]);
	}

	g_dl_close(dl);
}

// nvcc -shared -Xcompiler -fPIC test.cu -o libtest.so
