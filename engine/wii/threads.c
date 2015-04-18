/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#include <stdlib.h>
#include <ogcsys.h>
#include <time.h>
#include "threads.h"

#define LWP_RETCODE(X) ((X) == 0 ? 0 : -1)

struct bor_thread {
	lwp_t thread;
	int (*run)(void *);
	void *data;
};

static void *run_thread(void *data)
{
	bor_thread *thread = (bor_thread*)data;
	return (void*)thread->run(thread->data);
}

bor_thread *thread_create(int (*fn)(void *), const char *name, void *data)
{
	bor_thread *thread = malloc(sizeof(bor_thread));
	thread->run = fn;
	thread->data = data;
	LWP_CreateThread(&thread->thread, run_thread, thread, NULL, 0, 64);
	return thread;
}

void thread_join(bor_thread *thread)
{
	void *status;
	LWP_JoinThread(thread->thread, &status);
	free(thread);
}

bor_mutex *mutex_create(void)
{
	mutex_t *mutex = malloc(sizeof(mutex_t));
	if (LWP_MutexInit(mutex, 0) != 0) return NULL;
	return mutex;
}

void mutex_destroy(bor_mutex *mutex)
{
	LWP_MutexDestroy(*mutex);
	free(mutex);
}

int mutex_lock(bor_mutex *mutex)
{
	return LWP_RETCODE(LWP_MutexLock(*mutex));
}

int mutex_unlock(bor_mutex *mutex)
{
	return LWP_RETCODE(LWP_MutexUnlock(*mutex));
}

bor_cond *cond_create()
{
	cond_t *cond = malloc(sizeof(cond_t));
	if (LWP_CondInit(cond) != 0) return NULL;
	return cond;
}

void cond_destroy(bor_cond *cond)
{
	LWP_CondDestroy(*cond);
	free(cond);
}

int cond_signal(bor_cond *cond)
{
	return LWP_RETCODE(LWP_CondSignal(*cond));
}

int cond_broadcast(bor_cond *cond)
{
	return LWP_RETCODE(LWP_CondBroadcast(*cond));
}

int cond_wait(bor_cond *cond, bor_mutex *mutex)
{
	return LWP_RETCODE(LWP_CondWait(*cond, *mutex));
}

int cond_wait_timed(bor_cond *cond, bor_mutex *mutex, int ms)
{
	struct timespec time_to_wait;
	time_to_wait.tv_sec = ms / 1000;
	time_to_wait.tv_nsec = ((long)(ms % 1000)) * 1000000;
	return LWP_RETCODE(LWP_CondTimedWait(*cond, *mutex, &time_to_wait));
}
