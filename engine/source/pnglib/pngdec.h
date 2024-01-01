/*
 * OpenBOR - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef PNGDEC_H
#define PNGDEC_H

s_screen *pngToScreen(const void *data);

#ifdef SDL
#include "SDL.h"
SDL_Surface *pngToSurface(const void *data);
#endif

#endif

