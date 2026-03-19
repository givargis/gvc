// Copyright (c) Tony Givargis, 2024-2026

#include "g_spinlock.h"

#if defined(__x86_64__)
#   define cpu_relax() __builtin_ia32_pause()
#elif defined(__aarch64__)
#   define cpu_relax() __asm__ __volatile__ ("wfe" : : : "memory")
#else
#   error "unsupported architecture"
#endif

void
g_spinlock_lock(g_spinlock_t *lock)
{
	int n = 1;
	while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE)) {
		for (int i=0; i<n; ++i) {
			cpu_relax();
		}
		if (n < 1024) {
			n <<= 1;
		}
	}
}

void
g_spinlock_unlock(g_spinlock_t *lock)
{
	__atomic_clear(lock, __ATOMIC_RELEASE);
}
