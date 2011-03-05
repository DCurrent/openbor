#include "sdlpng.h"

SDL_Surface* pngToSurface(void* data, size_t size) {
	SDL_Surface* result;
	SDL_RWops* png;
	png = SDL_RWFromConstMem(data, size);
	result = IMG_LoadPNG_RW(png);
	SDL_FreeRW(png);
	return result;
}