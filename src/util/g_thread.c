// Copyright (c) Tony Givargis, 2024-2026

#include <pthread.h>

#include "g_thread.h"

struct g_thread {
	int good;
	void *ctx;
	pthread_t pthread;
	g_thread_fnc_t fnc;
};

struct g_mutex {
	pthread_mutex_t mutex;
};

struct g_cond {
	g_mutex_t mutex;
	pthread_cond_t cond;
};

static void *
_thread_(void *ctx)
{
	struct g_thread *thread;

	thread = (struct g_thread *)ctx;
	thread->fnc(thread->ctx);
	return NULL;
}

g_thread_t
g_thread_open(g_thread_fnc_t fnc, void *ctx)
{
	struct g_thread *thread;

	assert( fnc );

	if (!(thread = g_malloc(sizeof (struct g_thread)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(thread, 0, sizeof (struct g_thread));
	thread->ctx = ctx;
	thread->fnc = fnc;
	thread->good = 1;
	if (pthread_create(&thread->pthread, NULL, _thread_, thread)) {
		thread->good = 0;
		g_thread_close(thread);
		G_TRACE("system failure detected");
		return NULL;
	}
	return thread;
}

void
g_thread_close(g_thread_t thread)
{
	if (thread) {
		if (thread->good) {
			pthread_join(thread->pthread, NULL);
		}
		memset(thread, 0, sizeof (struct g_thread));
		g_free(thread);
	}
}

int
g_thread_good(g_thread_t thread)
{
	return thread && thread->good;
}

g_mutex_t
g_mutex_open(void)
{
	struct g_mutex *mutex;

	if (!(mutex = g_malloc(sizeof (struct g_mutex)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(mutex, 0, sizeof (struct g_mutex));
	if (pthread_mutex_init(&mutex->mutex, NULL)) {
		g_mutex_close(mutex);
		G_TRACE("system failure detected");
		return NULL;
	}
	return mutex;
}

void
g_mutex_close(g_mutex_t mutex)
{
	if (mutex) {
		pthread_mutex_destroy(&mutex->mutex);
		memset(mutex, 0, sizeof (struct g_mutex));
		g_free(mutex);
	}
}

void
g_mutex_lock(g_mutex_t mutex)
{
	assert( mutex );

	if (pthread_mutex_lock(&mutex->mutex)) {
		G_TRACE("system failure detected (abort)");
		abort();
	}
}

void
g_mutex_unlock(g_mutex_t mutex)
{
	assert( mutex );

	if (pthread_mutex_unlock(&mutex->mutex)) {
		G_TRACE("system failure detected (abort)");
		abort();
	}
}

g_cond_t
g_cond_open(g_mutex_t mutex)
{
	struct g_cond *cond;

	assert( mutex );

	if (!(cond = g_malloc(sizeof (struct g_cond)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(cond, 0, sizeof (struct g_cond));
	cond->mutex = mutex;
	if (pthread_cond_init(&cond->cond, NULL)) {
		g_cond_close(cond);
		G_TRACE("system failure detected");
		return NULL;
	}
	return cond;
}

void
g_cond_close(g_cond_t cond)
{
	if (cond) {
		pthread_cond_destroy(&cond->cond);
		memset(cond, 0, sizeof (struct g_cond));
		g_free(cond);
	}
}

void
g_cond_signal(g_cond_t cond)
{
	assert( cond );

	if (pthread_cond_signal(&cond->cond)) {
		G_TRACE("system failure detected (abort)");
		abort();
	}
}

void
g_cond_wait(g_cond_t cond)
{
	assert( cond );

	if (pthread_cond_wait(&cond->cond, &cond->mutex->mutex)) {
		G_TRACE("system failure detected (abort)");
		abort();
	}
}
