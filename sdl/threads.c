/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#include "sdlport.h"
#include "SDL_thread.h"

#include "threads.h"

bor_thread *thread_create(int (*fn)(void *), const char *name, void *data)
{
	return SDL_CreateThread(fn, name, data);
}

void thread_join(bor_thread *thread)
{
	SDL_WaitThread(thread, NULL);
}

bor_mutex *mutex_create(void)
{
	return SDL_CreateMutex();
}

void mutex_destroy(bor_mutex *mutex)
{
	SDL_DestroyMutex(mutex);
}

int mutex_lock(bor_mutex *mutex)
{
	return SDL_mutexP(mutex);
}

int mutex_unlock(bor_mutex *mutex)
{
	return SDL_mutexV(mutex);
}

bor_cond *cond_create(void)
{
	return SDL_CreateCond();
}

void cond_destroy(bor_cond *cond)
{
	SDL_DestroyCond(cond);
}

int cond_signal(bor_cond *cond)
{
	return SDL_CondSignal(cond);
}

int cond_broadcast(bor_cond *cond)
{
	return SDL_CondBroadcast(cond);
}

int cond_wait(bor_cond *cond, bor_mutex *mutex)
{
	return SDL_CondWait(cond, mutex);
}

int cond_wait_timed(bor_cond *cond, bor_mutex *mutex, int ms)
{
	return SDL_CondWaitTimeout(cond, mutex, ms);
}
