/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/////////////////////////////////////////////////////////////////////////////

#define I_AM_TRACEMALLOC

#ifdef PSP
#include <pspsysmem.h>
#endif

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "ram.h"
#include "globals.h"
#include "tracemalloc.h"

/////////////////////////////////////////////////////////////////////////////

#ifdef RAM_DEBUG
static ptrdiff_t *tracehead = NULL;
size_t tracemalloc_total = 0;

#if defined(GP2X) && !defined(WIZ)
	#define TRACE_BYTES 20
	#define TRACE_SIZE 4
#else
	#define TRACE_BYTES 4 * sizeof(void*)
	#define TRACE_SIZE 3
#endif

/////////////////////////////////////////////////////////////////////////////

static void tracemalloc_dump_collect(ptrdiff_t *p, size_t *len, size_t *nalloc)
{
    ptrdiff_t name = p[2];
    *len = 0;
    *nalloc = 0;
    for(; p; p = (ptrdiff_t*)(p[1]))
    {
		if(p[2] == name && p[TRACE_SIZE] > 0)
		{
			(*len) += p[TRACE_SIZE];
			(*nalloc) += 1;
			p[TRACE_SIZE] = -p[TRACE_SIZE];
		}
	}
}
#endif

int tracemalloc_dump(void)
{
#ifdef RAM_DEBUG
	size_t totalbytes = 0;
	ptrdiff_t *p, *pp;
	unsigned long long lc, nc;
	for(p = tracehead; p; p = (ptrdiff_t*)(p[1]))
	{
		if(p[TRACE_SIZE] > 0)
		{
			const char *name = (const char*)(p[2]);
			size_t len = 0;
			size_t nalloc = 0;
			tracemalloc_dump_collect(p, &len, &nalloc);
			lc = len; nc = nalloc;
			printf("%s: %llu bytes in %llu allocs\n", name, lc, nc);
			totalbytes += len;
		}
	}
	for(p = tracehead; p; pp = p, p = (ptrdiff_t*)(p[1]), free(pp))
		if(p[TRACE_SIZE] < 0) p[TRACE_SIZE] = -p[TRACE_SIZE];
    if(totalbytes)
	{
		lc = totalbytes;
		printf("Total Leaked Bytes %llu\n", lc);
		return 1;
	}
#endif
	return 0;
}

void *tracemalloc(const char *name, size_t len)
{
	ptrdiff_t *p;

#ifndef RAM_DEBUG
#if defined(GP2X) && !defined(WIZ)
	p = malloc(len + 1);
#else
	p = malloc(len);
#endif
#else
	p = malloc(TRACE_BYTES + len);
#endif

#if defined(GP2X) && !defined(WIZ)
	ptrdiff_t uRam = 0;
	if(!p)
	{
#ifdef RAM_DEBUG
		p = UpperMalloc(TRACE_BYTES + len);
#else
		p = UpperMalloc(1 + len);
#endif
		if(!p)
		{
			writeToLogFile("name: %s Requested: %d Bytes, Remaining, %lu Bytes\n", name, len, getFreeRam(BYTES));
			return NULL;
		}
		uRam = 1;
	}
#else
	if(!p)
	{
		writeToLogFile("name: %s Requested: %d Bytes, Remaining, %lu Bytes\n", name, len, getFreeRam(BYTES));
		return NULL;
	}
#endif

#ifdef RAM_DEBUG
	if(tracehead) tracehead[0] = (ptrdiff_t)p;
	p[0] = 0;
	p[1] = (ptrdiff_t)tracehead;
	p[2] = (ptrdiff_t)name;
#endif

#if defined(GP2X) && !defined(WIZ)
#ifdef RAM_DEBUG
	p[3] = uRam;
#else
	p[0] = uRam;
	return (void*)(++p);
#endif
#endif

#ifdef RAM_DEBUG
	p[TRACE_SIZE] = len;
	tracehead = p;
	tracemalloc_total += TRACE_BYTES + len;
	return (void*)(p + (TRACE_SIZE + 1));
#else
	return (void*)(p);
#endif
}

void tracefree(void *vp)
{
#ifdef RAM_DEBUG
	ptrdiff_t *p = NULL;
	ptrdiff_t *p_from_prev = NULL;
	ptrdiff_t *p_from_next = NULL;
	p = ((ptrdiff_t*)vp) - (TRACE_SIZE + 1);
	tracemalloc_total -= TRACE_BYTES + p[TRACE_SIZE];
	if(p == tracehead) p_from_prev = (void*)(&tracehead);
	else               p_from_prev = (ptrdiff_t*)(p[0] + sizeof(ptrdiff_t));
	p_from_next = (ptrdiff_t*)(p[1]);
	if(p_from_prev) *p_from_prev = p[1];
	if(p_from_next) *p_from_next = p[0];
#endif

#if defined(GP2X) && !defined(WIZ)
#ifdef RAM_DEBUG
	if(p[3]) UpperFree(p);
	else free(p);
#else
	if(((ptrdiff_t*)(vp))[0]) UpperFree(vp);
	else free(vp);
#endif
#else
#ifdef RAM_DEBUG
	free(p);
#else
	free(vp);
#endif
#endif
}

// Plombo Nov 21 2010: add realloc() functionality to tracelib
void *tracerealloc(void *vp, size_t len)
{
	if(!vp) // realloc(NULL, len) == malloc(len)
		return tracemalloc("tracerealloc", len);
	else if(len == 0) // realloc(vp, 0) == free(vp)
	{
		tracefree(vp);
		return NULL;
	}
	else // actually call realloc()
	{
#ifdef RAM_DEBUG
		char *p = ((char*)vp) - TRACE_BYTES;
		ptrdiff_t *vp2 = realloc(p, len + TRACE_BYTES);
		if(!vp2) return NULL;
		tracemalloc_total -= vp2[TRACE_SIZE];
		vp2[TRACE_SIZE] = len;
		tracemalloc_total += vp2[TRACE_SIZE];
		vp2 += TRACE_SIZE + 1;
		return (void*)vp2;
#else
		return realloc(vp, len);
#endif
	}
}

// Plombo 11/21/10: add a calloc() function to tracelib
void *tracecalloc(const char *name, size_t len)
{
	void *ptr = tracemalloc(name, len);
	if(ptr) memset(ptr, 0, len);
	return ptr;
}

