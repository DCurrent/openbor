/*
 * tracelib2 library.
 * (C) 2011 anallyst
 *
 * license to use and redistribute is granted under the same terms as OpenBOR itself.
 * as long as anallyst is granted to use and redistribute the OpenBOR source code,
 * and maintain his own fork.
 *
 */

#ifndef _TRACELIB2_H_
#define _TRACELIB2_H_

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#define MALLOC(z) tlmalloc(__LINE__, __FILE__, z)
#define FREE(z) tlfree(__LINE__, __FILE__, z)
#define REALLOC(y,z) tlrealloc(__LINE__, __FILE__, y, z)
#define CALLOC(y,z) tlcalloc(__LINE__, __FILE__, y, z)


typedef struct
{
    int line;
    char *file;
    size_t size;
    char *buf;
} tlInfo;

void tlinit(void);
void *tlmalloc(int line, char *file, size_t size);
void tlfree(int line, char *file, void *ptr);
void *tlrealloc(int line, char *file, void *ptr, size_t size);
void *tlcalloc(int line, char *file, size_t count, size_t size);
int tlstats(void);

#endif