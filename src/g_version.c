// Copyright (c) Tony Givargis, 2024-2026

#include "util/g_util.h"

#include "g_version.h"

void
g_version_logo(void)
{
	g_log("info: =====================================");
        g_log("info:    ____ __   _______");
        g_log("info:   / __ `/ | / / ___/");
        g_log("info:  / /_/ /| |/ / /__");
        g_log("info:  \\__, / |___/\\___/");
        g_log("info: /____/");
	g_log("info: %s", G_VERSION_COPYRIGHT);
	g_log("info: =====================================");
}
