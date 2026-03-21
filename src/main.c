// Copyright (c) Tony Givargis, 2024-2026

#include "util/g_util.h"
#include "lang/g_lang.h"
#include "cuda/g_cuda.h"

#include "g_version.h"

int
main(int argc, char *argv[])
{
	struct g_cuda_info info;

	(void)argc;
	(void)argv;

	g_util_init(0, 0, 0);
	g_version_logo();

	if (g_cuda_info_create(&info)) {
		G_TRACE("^");
		return -1;
	}
	g_cuda_info_print(&info);
	g_cuda_info_destroy(&info);

	if (g_cuda_execute()) {
		G_TRACE("^");
		return -1;
	}
	return 0;
}
