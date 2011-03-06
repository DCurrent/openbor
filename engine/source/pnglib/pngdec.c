/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <png.h>
#include "types.h"
#include "screen.h"
#include "tracemalloc.h"
#include <assert.h>

#ifdef SDL
#include "SDL.h"
#endif

#if PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR == 2 && PNG_LIBPNG_VER_RELEASE < 9
void png_set_expand_gray_1_2_4_to_8(png_structp png_ptr) { png_set_gray_1_2_4_to_8(png_ptr); }
#endif

static void png_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
}

static void png_read_fn(png_structp pngp, png_bytep outp, png_size_t size)
{
    char** ptr = (char**)png_get_io_ptr(pngp);
    memcpy(outp, *ptr, size);
    *ptr += size;
}

s_screen* pngToScreen(const void* data)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type, y;
	u32* line;
	s_screen* image;
	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr == NULL) goto error;
	png_set_error_fn(png_ptr, (png_voidp) NULL, (png_error_ptr) NULL, png_warning_fn);
	info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL) goto error2;
	if(setjmp(png_jmpbuf(png_ptr))) goto error2;

	png_set_read_fn(png_ptr, &data, png_read_fn);
	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);
	if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
	if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

	image = allocscreen(width, height, PIXEL_32);
	if(image == NULL) goto error2;

	line = (u32*)image->data;
	for(y=0; y<height; y++)
	{
		png_read_row(png_ptr, (u8*) line, NULL);
		line += width;
	}
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	return image;
	
error2:
	png_destroy_read_struct(&png_ptr, NULL, NULL);
error:
	return NULL;
}

#ifdef SDL
SDL_Surface* pngToSurface(const void* data)
{
	unsigned char *sp;
    char *dp;
    int width, height, linew;
    int h;
    SDL_Surface* ds = NULL;
    s_screen* src = pngToScreen(data);

	if(src == NULL) return NULL;
	assert(src->pixelformat == PIXEL_32);
	
    width = src->width;
    height = src->height;
    h = height;

    sp = (unsigned char*)src->data;
    ds = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, 0xff, 0xff00, 0xff0000, 0);
    dp = ds->pixels;

    linew = width*4;

    do{
        memcpy(dp, sp, linew);
        sp += linew;
        dp += ds->pitch;
    }while(--h);
    
    freescreen(&src);

    return ds;
}
#endif

