/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#ifndef THREADS_H
#define THREADS_H

#if WII
#include <ogcsys.h>
struct bor_thread;
typedef struct bor_thread bor_thread;
typedef mutex_t bor_mutex;
typedef cond_t bor_cond;
#elif SDL
#include "SDL.h"
#include "SDL_thread.h"
typedef SDL_Thread bor_thread;
typedef SDL_mutex bor_mutex;
typedef SDL_cond bor_cond;
#else
#error threads not supported on this platform
#endif

// creates a new thread that returns when fn returns
bor_thread *thread_create(int (*fn)(void *), const char *name, void *data);

// waits for thread to finish
void thread_join(bor_thread *thread);

// create a mutex; returns NULL on error
bor_mutex *mutex_create(void);

// destroys a mutex
void mutex_destroy(bor_mutex *mutex);

// tries to lock a mutex; returns 0 on success and -1 on error
int mutex_lock(bor_mutex *mutex);

// unlocks a mutex; returns 0 on success and -1 on error
int mutex_unlock(bor_mutex *mutex);

// creates a new condition variable; returns NULL on error
bor_cond *cond_create(void);

// destroys and frees a condition variable
void cond_destroy(bor_cond *cond);

// signal a single thread waiting on this condition variable to wake up
int cond_signal(bor_cond *cond);

// broadcast to all threads waiting on this condition variable to wake up
int cond_broadcast(bor_cond *cond);

// make the current thread wait on the condition variable
// mutex must be locked; this function will unlock it when called and re-lock it before returning
int cond_wait(bor_cond *cond, bor_mutex *mutex);

// make the current thread wait on the condition variable with a timeout
// mutex must be locked; this function will unlock it when called and re-lock it before returning
int cond_wait_timed(bor_cond *cond, bor_mutex *mutex, int ms);

#endif // ifndef THREADS_H
