/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "types.h"
#ifdef SDL
#include <SDL.h>
#endif

s_screen* xpmToScreen(char *array[]);

#ifdef SDL
SDL_Surface* xpmToSurface(char *array[]);
#endif

