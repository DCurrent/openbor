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

/////////////////////////////////////////////////////////////////////////////

extern unsigned long tracemalloc_total;

/////////////////////////////////////////////////////////////////////////////

void *tracemalloc(const char *name, int len);
void *tracecalloc(const char *name, int len);
void *tracerealloc(void *p, int len, int oldlen);
void tracefree(void *p);
int tracemalloc_dump(void);

/////////////////////////////////////////////////////////////////////////////

#endif

