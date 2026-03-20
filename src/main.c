// Copyright (c) Tony Givargis, 2024-2026

#include "util/g_util.h"
#include "lang/g_lang.h"
#include "cuda/g_cuda.h"

#include "g_version.h"

int
main(int argc, char *argv[])
{
	struct g_cuda_info_device *device;
	struct g_cuda_info info;

	(void)argc;
	(void)argv;

	g_util_init(0, 0, 0);
	g_version_logo();

	if (g_cuda_info_create(&info)) {
		G_TRACE("^");
		return -1;
	}

	for (int i=0; i<info.size; ++i) {
		device = &info.devices[i];
		g_log("info: %s {", device->name);
		g_log("info:   hardware {");
		g_log("info:     %d * %d ",
		      device->hardware[0],
		      device->hardware[1]);
		g_log("info:   }");
		g_log("info:   threads {");
		g_log("info:     %d * %d * %d (<= %d)",
		      device->threads[0],
		      device->threads[1],
		      device->threads[2],
		      device->threads[3]);
		g_log("info:   }");
		g_log("info:   blocks {");
		g_log("info:     %d * %d * %d",
		      device->blocks[0],
		      device->blocks[1],
		      device->blocks[2]);
		g_log("info:   }");
		g_log("info: }");


	}

	g_cuda_info_destroy(&info);
	return 0;
}
