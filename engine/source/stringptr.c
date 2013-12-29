/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "stringptr.h"
#include <assert.h>
#include <string.h>

stringptr *new_string(size_t size)
{
    stringptr *result = malloc(sizeof(stringptr) + size + 1);
    if (result == NULL)
    {
        return NULL;
    }
    result->ptr = (char *)result + sizeof(stringptr);
    result->size = size;
    *((char *) (result->ptr + size)) = '\0';
    return result;
}

void free_string(stringptr *string)
{
    free(string);
}
