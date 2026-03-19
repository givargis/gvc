// Copyright (c) Tony Givargis, 2024-2026

#ifndef _G_SPINLOCK_H_
#define _G_SPINLOCK_H_

#include "g_core.h"

#define G_SPINLOCK_INITIALIZER ( (g_spinlock_t)0 )

typedef unsigned char g_spinlock_t;

void g_spinlock_lock(g_spinlock_t *lock);

void g_spinlock_unlock(g_spinlock_t *lock);

#endif
