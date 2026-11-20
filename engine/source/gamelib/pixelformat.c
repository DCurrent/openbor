/*
 * OpenBOR - http://www.chronocrash.com
 * All rights reserved, see LICENSE in OpenBOR root for details.
 * Copyright (c) OpenBOR Team
 */

#include "types.h"
#include "globals.h"
#include "gpu.h"

/* Indexed artwork owns RGB32 palettes; scenes have their own RGB32 format. */
int pixelformat = PIXEL_x8;
/* Unsupported format identities must never supply a pixel stride. */
int pixelbytes[(int)5] = {1, 1, 0, 0, 4};

unsigned colour32(unsigned char r, unsigned char g, unsigned char b)
{
#if REVERSE_COLOR
    return ((r << 16) | (g << 8) | b);
#else
    return ((b << 16) | (g << 8) | r);
#endif
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Retain byte-channel formulas solely for building GPU lookup textures.
 * Per-pixel RGB32 blending and global software material state are retired.
 */
#define _multiply(c1,c2) (((c1)*(c2))>>8)
#define _screen(c1,c2) ((((c1)^255)*((c2)^255)/255)^255)
#define _hardlight(c1,c2) ((c1)<128?_multiply((c1)<<1,(c2)):_screen(((c1)-128)<<1,(c2)))
#define _overlay(c1,c2) ((c2)<128?_multiply((c2)<<1,(c1)):_screen(((c2)-128)<<1,(c1)))
#define _dodge(c1,c2) (((c2)<<8)/(256-(c1)))

unsigned blend_multiply(unsigned color1, unsigned color2)
{
    return _multiply(color1, color2);
}

unsigned blend_screen(unsigned color1, unsigned color2)
{
    return _screen(color1, color2);
}

unsigned blend_overlay(unsigned color1, unsigned color2)
{
    return _overlay(color1, color2);
}

unsigned blend_hardlight(unsigned color1, unsigned color2)
{
    return _hardlight(color1, color2);
}

unsigned blend_dodge(unsigned color1, unsigned color2)
{
    unsigned c = _dodge(color1, color2);
    return c > 255 ? 255 : c;
}

unsigned blend_half(unsigned color1, unsigned color2)
{
    return (color1 + color2) >> 1;
}

unsigned char *create_multiply32_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(256 * 256);
    for(i = 0; i < 256; i++)
        for(j = 0; j < 256; j++)
        {
            tbl[(i << 8) | j] = (unsigned char)blend_multiply(i, j);
        }

    return tbl;
}

unsigned char *create_screen32_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(256 * 256);
    for(i = 0; i < 256; i++)
        for(j = 0; j < 256; j++)
        {
            tbl[(i << 8) | j] = (unsigned char)blend_screen(i, j);
        }

    return tbl;
}

unsigned char *create_overlay32_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(256 * 256);
    for(i = 0; i < 256; i++)
        for(j = 0; j < 256; j++)
        {
            tbl[(i << 8) | j] = (unsigned char)blend_overlay(i, j);
        }

    return tbl;
}

unsigned char *create_hardlight32_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(256 * 256);
    for(i = 0; i < 256; i++)
        for(j = 0; j < 256; j++)
        {
            tbl[(i << 8) | j] = (unsigned char)blend_hardlight(i, j);
        }

    return tbl;
}

unsigned char *create_dodge32_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(256 * 256);
    for(i = 0; i < 256; i++)
        for(j = 0; j < 256; j++)
        {
            tbl[(i << 8) | j] = (unsigned char)blend_dodge(i, j);
        }
    return tbl;
}

unsigned char *create_half32_tbl()
{
    unsigned i, j;
    unsigned char *tbl = (unsigned char *)malloc(256 * 256);
    for(i = 0; i < 256; i++)
        for(j = 0; j < 256; j++)
        {
            tbl[(i << 8) | j] = (unsigned char)blend_half(i, j);
        }

    return tbl;
}

unsigned char *blendtables[MAX_BLENDINGS] = {NULL, NULL, NULL, NULL, NULL, NULL};

/* Replace channel lookup sources and invalidate their resident GPU textures. */
void set_blendtables(unsigned char *tables[])
{
    int i;
    for(i = 0; i < MAX_BLENDINGS; i++)
    {
        blendtables[i] = tables[i];
    }
    /* GPU blend lookup textures must track replacements of these tables. */
    gpu_invalidate_tables();
}
