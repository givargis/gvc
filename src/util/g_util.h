// Copyright (c) Tony Givargis, 2024-2026

#ifndef _UTIL_H_
#define _UTIL_H_

#include "g_base64.h"
#include "g_bigint.h"
#include "g_bitset.h"
#include "g_core.h"
#include "g_csv.h"
#include "g_dl.h"
#include "g_dir.h"
#include "g_ec.h"
#include "g_execute.h"
#include "g_fft.h"
#include "g_file.h"
#include "g_hash.h"
#include "g_json.h"
#include "g_map.h"
#include "g_mlp.h"
#include "g_network.h"
#include "g_random.h"
#include "g_sha3.h"
#include "g_spinlock.h"
#include "g_thread.h"
#include "g_vector.h"

void g_util_init(int noinfo, int notrace, int noerror);

#endif
