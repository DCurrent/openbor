/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef SDLPORT_H
#define SDLPORT_H

#include <SDL.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include "globals.h"

#if GP2X || LINUX || OPENDINGUX || SYMBIAN
#define stricmp  strcasecmp
#define strnicmp strncasecmp
#endif

#if SDL_VERSION_ATLEAST(2,0,0)
#define SDL2 1
#endif

#if GP2X || DARWIN || OPENDINGUX || ANDROID
#define SKIP_CODE
#endif

#ifdef ANDROID
#define MAXTOUCHB 13
#endif

//#define MEMTEST 1

#if _POSIX_C_SOURCE >= 199309L
void _usleep(u32 usec);
#define usleep _usleep
#endif

void initSDL();
void borExit(int reset);
void openborMain(int argc, char** argv);

extern char packfile[128];
extern char paksDir[128];
extern char savesDir[128];
extern char logsDir[128];
extern char screenShotsDir[128];

#endif
