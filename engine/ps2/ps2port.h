/////////////////////////////////////////////////////////////////////////////
//
// PS2-specific port stuff
//
/////////////////////////////////////////////////////////////////////////////

#ifndef PS2PORT_H
#define PS2PORT_H

/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <kernel.h>
#include <tamtypes.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>

/////////////////////////////////////////////////////////////////////////////

#define PS2PORT

#define PS2RELEASE

///////////////////////////////////////////////////////////////////////////// 

void LoadModule(char *path, int argc, char *argv);

#define gamelib_long int

unsigned readlsb32(const unsigned char *src);

//#define PS2_BINARY 0
//#define PS2_RDONLY 0

//int ps2open(const char *filename, int mode);
//int ps2read(int fd, void *buf, int len);
//void ps2close(int fd);
//int ps2lseek(int fd, int n, int whence);

// NOT REENTRANT
const char *ps2gethostfilename(const char *filename);

#if 0
#define dbg_printf(args...) printf(args)
#else
#define dbg_printf(args...)
#endif

#include "tracemalloc.h"

#ifndef I_AM_TRACEMALLOC
#ifdef PS2RELEASE
#define tracemalloc(x,y) malloc(y)
#define tracefree(x) free(x)
#else
#define malloc dont_use_malloc_dammit
#define free dont_use_free_dammit
#endif
#endif

#include "ps2sdr.h"
#define SB_setvolume(a,b) ps2sdr_setvolume(((unsigned)(b))*0x7FF)

void debug_printf(char *, ...);

extern int blocking;

/////////////////////////////////////////////////////////////////////////////

#endif

