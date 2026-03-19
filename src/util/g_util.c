// Copyright (c) Tony Givargis, 2024-2026

#include <stdlib.h>

#include "g_util.h"

void
g_util_init(int noinfo, int notrace, int noerror)
{
	g_init(noinfo, notrace, noerror);
	g_ec_init();
}
