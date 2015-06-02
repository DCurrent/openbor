/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#ifndef SAFEALLOC_H
#define SAFEALLOC_H

#include <stdlib.h>
#include <string.h> // for strlen
#include "utils.h" // for checkAlloc

#define MALLOCLIKE __attribute__((__malloc__))

static inline void *safeRealloc(void *ptr, size_t size, const char *func, const char *file, int line)
{
    return checkAlloc(realloc(ptr, size), size, func, file, line);
}

// attributes can only be declared on function declarations, so declare these before defining them
static inline void *safeMalloc(size_t size, const char *func, const char *file, int line) MALLOCLIKE;
static inline void *safeCalloc(size_t nmemb, size_t size, const char *func, const char *file, int line) MALLOCLIKE;
static inline void *safeStrdup(const char *str, const char *func, const char *file, int line) MALLOCLIKE;

static inline void *safeMalloc(size_t size, const char *func, const char *file, int line)
{
    return checkAlloc(malloc(size), size, func, file, line);
}

static inline void *safeCalloc(size_t nmemb, size_t size, const char *func, const char *file, int line)
{
    return checkAlloc(calloc(nmemb, size), size, func, file, line);
}

static inline void *safeStrdup(const char *str, const char *func, const char *file, int line)
{
    // reimplement strdup to avoid doing strlen(str) twice
    int size = strlen(str) + 1;
    char *newString = (char *) checkAlloc(malloc(size), size, func, file, line);
    memcpy(newString, str, size);
    return newString;
}

#define realloc(ptr, size) safeRealloc(ptr, size, __func__, __FILE__, __LINE__)
#define malloc(size) safeMalloc(size, __func__, __FILE__, __LINE__)
#define calloc(nmemb, size) safeCalloc(nmemb, size, __func__, __FILE__, __LINE__)
#define strdup(str) safeStrdup(str, __func__, __FILE__, __LINE__)

#endif // !defined SAFEALLOC_H
