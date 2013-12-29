/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef _STRINGPTR_H_
#define _STRINGPTR_H_

#include <stdlib.h>
#include <string.h>

typedef struct
{
    char *ptr;
    size_t size;
} stringptr;

stringptr *new_string(size_t size);
void free_string(stringptr *string);

#endif
