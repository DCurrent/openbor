#ifndef _SDLPNG_H_
#define _SDLPNG_H_

#include "SDL.h"
#include "SDL_image.h"

SDL_Surface* pngToSurface(void* data, size_t size);

#endif