/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <png.h>
#include <malloc.h>
#include "types.h"
#include "screen.h"
#include <assert.h>

void savepng(const char *filename, s_screen *screen, u8 *pal)
{
    u32 *vram32;
    u16 *vram16;
    int i, x, y;
    png_structp png_ptr;
    png_infop info_ptr;
    FILE *fp;
    u8 *line;

    fp = fopen(filename, "wb");
    if(!fp)
    {
        return;
    }
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr)
    {
        return;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr)
    {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fclose(fp);
        return;
    }
    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, screen->width, screen->height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);
    line = (u8 *)malloc(screen->width * 3);

    vram32 = (u32 *)screen->data;
    vram16 = (u16 *)screen->data;

    for(y = 0; y < screen->height; y++)
    {
        for(i = 0, x = 0; x < screen->width; x++)
        {
            u32 color = 0;
            u8 r = 0, g = 0, b = 0;
            switch (screen->pixelformat)
            {
            case PIXEL_8:
            case PIXEL_x8:
                color = screen->data[x + y * screen->width];
                r = pal[color * 3];
                g = pal[color * 3 + 1];
                b = pal[color * 3 + 2];
                break;
            case PIXEL_16:
                color = vram16[x + y * screen->width];
                r = (color & 0x1f) << 3;
                g = ((color >> 5) & 0x3f) << 2;
                b = ((color >> 11) & 0x1f) << 3;
                break;
            case PIXEL_32:
                color = vram32[x + y * screen->width];
                r = color & 0xff;
                g = (color >> 8) & 0xff;
                b = (color >> 16) & 0xff;
                break;
            }
            line[i++] = r;
            line[i++] = g;
            line[i++] = b;
        }
        png_write_row(png_ptr, line);
    }
    free(line);
    line = NULL;
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    fclose(fp);
}

