/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef STRINGPTR_H
#define STRINGPTR_H

#include <string.h>

typedef struct {
	char *ptr;
	size_t size;
} stringptr;

stringptr* new_string(size_t size);
void free_string(stringptr* string);

#endif
