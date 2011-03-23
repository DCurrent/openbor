/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef SDLPORT_H
#define SDLPORT_H

#include <SDL.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include "globals.h"

#if GP2X || LINUX || DINGOO || SYMBIAN
#define stricmp  strcasecmp
#define strnicmp strncasecmp
#endif

#if SDL_VERSION_ATLEAST(1,3,0)
#define SDL13
#endif

#ifdef SDL13
#define SDL_FreeVideoSurface(X)
#define SDL_FreeAndNullVideoSurface(X)
#else
#define SDL_FreeVideoSurface(X) SDL_FreeSurface(X)
#define SDL_FreeAndNullVideoSurface(X) { SDL_FreeSurface(X); X=NULL; }
#endif

//#define MEMTEST 1

void initSDL();
void borExit(int reset);
void openborMain(int argc, char** argv);

extern char packfile[128];
extern char paksDir[128];
extern char savesDir[128];
extern char logsDir[128];
extern char screenShotsDir[128];

#endif
