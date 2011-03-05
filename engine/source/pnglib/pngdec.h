/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef PNGDEC_H
#define PNGDEC_H

s_screen* pngToScreen(const void* data);

#ifdef SDL
#include "SDL.h"
SDL_Surface* pngToSurface(const void* data);
#endif

#endif

