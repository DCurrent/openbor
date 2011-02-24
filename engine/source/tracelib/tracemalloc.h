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

/////////////////////////////////////////////////////////////////////////////

extern size_t tracemalloc_total;

/////////////////////////////////////////////////////////////////////////////

void *tracemalloc(const char *name, size_t len);
void *tracecalloc(const char *name, size_t len);
void *tracerealloc(void *p, size_t len);
void tracefree(void *p);
int tracemalloc_dump(void);

/////////////////////////////////////////////////////////////////////////////

#endif

