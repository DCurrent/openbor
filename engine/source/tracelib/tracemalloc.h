/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/////////////////////////////////////////////////////////////////////////////

#ifndef TRACEMALLOC_H
#define TRACEMALLOC_H

#include <string.h>
#include <stdlib.h>

#ifdef DEBUG
#ifndef NO_RAM_DEBUGGER
#define RAM_DEBUG 1
#endif
#endif

/////////////////////////////////////////////////////////////////////////////

extern size_t tracemalloc_total;

/////////////////////////////////////////////////////////////////////////////
#ifndef RAM_DEBUG
#define tracemalloc(a,b) malloc(b)
#define tracerealloc(a,b) realloc(a,b)
#define tracefree(a) free(a)
#define tracecalloc(a,b) calloc(1,b)
#else
void *tracemalloc(const char *name, size_t len);
void *tracecalloc(const char *name, size_t len);
void *tracerealloc(void *p, size_t len);
void tracefree(void *p);
#endif
int tracemalloc_dump(void);

/////////////////////////////////////////////////////////////////////////////

#endif

