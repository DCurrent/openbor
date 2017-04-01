/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

/* This code was derived from code carrying the following copyright notices:

 *  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source distribution.

 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

 * Copyright (c) 1995 Erik Corry
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL ERIK CORRY BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
 * SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF
 * THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF ERIK CORRY HAS BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ERIK CORRY SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND ERIK CORRY HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT,
 * UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

 * Portions of this software Copyright (c) 1995 Brown University.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement
 * is hereby granted, provided that the above copyright notice and the
 * following two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF BROWN
 * UNIVERSITY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * BROWN UNIVERSITY SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND BROWN UNIVERSITY HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include <stdlib.h>
#include <stdint.h>
#include "globals.h"
#include "yuv.h"

static int *colortab;
static uint32_t *rgb_2_pix;
static int bytes_per_pixel;
static int yuv_initialized = 0;

/*
 * How many 1 bits are there in the Uint32.
 * Low performance, do not call often.
 */
static int number_of_bits_set(uint32_t a)
{
    if(!a) return 0;
    if(a & 1) return 1 + number_of_bits_set(a >> 1);
    return(number_of_bits_set(a >> 1));
}

/*
 * How many 0 bits are there at least significant end of Uint32.
 * Low performance, do not call often.
 */
static int free_bits_at_bottom(uint32_t a)
{
    /* assume char is 8 bits */
    if(!a) return sizeof(uint32_t) * 8;
    if(((int32_t)a) & 1l) return 0;
    return 1 + free_bits_at_bottom ( a >> 1);
}

yuv_frame *yuv_frame_create(int width, int height)
{
    yuv_frame *frame = malloc(sizeof(yuv_frame));
    frame->lum = malloc(width * height);
    frame->cr = malloc(width * height / 2);
    frame->cb = malloc(width * height / 2);
    return frame;
}

void yuv_frame_destroy(yuv_frame *frame)
{
    if(frame == NULL) return;
    free(frame->lum);
    free(frame->cr);
    free(frame->cb);
    free(frame);
}

void yuv_init(int pixelbytes)
{
    int *Cr_r_tab;
    int *Cr_g_tab;
    int *Cb_g_tab;
    int *Cb_b_tab;
    uint32_t *r_2_pix_alloc;
    uint32_t *g_2_pix_alloc;
    uint32_t *b_2_pix_alloc;
    int i;
    int CR, CB;
    uint32_t Rmask, Gmask, Bmask;

    yuv_clear();
    yuv_initialized = 1;

    bytes_per_pixel = pixelbytes;

    colortab = (int *)malloc(4*256*sizeof(int));
    Cr_r_tab = &colortab[0*256];
    Cr_g_tab = &colortab[1*256];
    Cb_g_tab = &colortab[2*256];
    Cb_b_tab = &colortab[3*256];
    rgb_2_pix = (uint32_t *)malloc(3*768*sizeof(uint32_t));
    r_2_pix_alloc = &rgb_2_pix[0*768];
    g_2_pix_alloc = &rgb_2_pix[1*768];
    b_2_pix_alloc = &rgb_2_pix[2*768];
    if (!colortab || !rgb_2_pix)
    {
        if (colortab) free(colortab);
        if (rgb_2_pix) free(rgb_2_pix);
        return;
    }

    /* Generate the tables for the display surface */
    for (i=0; i<256; i++) {
        /* Gamma correction (luminescence table) and chroma correction
           would be done here.  See the Berkeley mpeg_play sources.
        */
        CB = CR = (i-128);
        Cr_r_tab[i] = (int) ( (0.419/0.299) * CR);
        Cr_g_tab[i] = (int) (-(0.299/0.419) * CR);
        Cb_g_tab[i] = (int) (-(0.114/0.331) * CB); 
        Cb_b_tab[i] = (int) ( (0.587/0.331) * CB);
    }

    /* 
     * Set up entries 0-255 in rgb-to-pixel value tables.
     */
    if (bytes_per_pixel == 2)
    {
        Rmask = 0xf800;
        Gmask = 0x7e0;
        Bmask = 0x1f;
    }
    else
    {
        assert(bytes_per_pixel == 4);
        Rmask = 0xff0000;
        Gmask = 0x00ff00;
        Bmask = 0x0000ff;
    }
    for ( i=0; i<256; ++i ) {
        r_2_pix_alloc[i+256] = i >> (8 - number_of_bits_set(Rmask));
        r_2_pix_alloc[i+256] <<= free_bits_at_bottom(Rmask);
        g_2_pix_alloc[i+256] = i >> (8 - number_of_bits_set(Gmask));
        g_2_pix_alloc[i+256] <<= free_bits_at_bottom(Gmask);
        b_2_pix_alloc[i+256] = i >> (8 - number_of_bits_set(Bmask));
        b_2_pix_alloc[i+256] <<= free_bits_at_bottom(Bmask);
    }

    /*
     * If we have 16-bit output depth, then we double the value
     * in the top word. This means that we can write out both
     * pixels in the pixel doubling mode with one op. It is 
     * harmless in the normal case as storing a 32-bit value
     * through a short pointer will lose the top bits anyway.
     */
    if (bytes_per_pixel == 2) {
        for ( i=0; i<256; ++i ) {
            r_2_pix_alloc[i+256] |= (r_2_pix_alloc[i+256]) << 16;
            g_2_pix_alloc[i+256] |= (g_2_pix_alloc[i+256]) << 16;
            b_2_pix_alloc[i+256] |= (b_2_pix_alloc[i+256]) << 16;
        }
    }

    /*
     * Spread out the values we have to the rest of the array so that
     * we do not need to check for overflow.
     */
    for ( i=0; i<256; ++i ) {
        r_2_pix_alloc[i] = r_2_pix_alloc[256];
        r_2_pix_alloc[i+512] = r_2_pix_alloc[511];
        g_2_pix_alloc[i] = g_2_pix_alloc[256];
        g_2_pix_alloc[i+512] = g_2_pix_alloc[511];
        b_2_pix_alloc[i] = b_2_pix_alloc[256];
        b_2_pix_alloc[i+512] = b_2_pix_alloc[511];
    }
}

void yuv_clear(void)
{
    free(colortab);
    free(rgb_2_pix);
    colortab = NULL;
    rgb_2_pix = NULL;
    yuv_initialized = 0;
}

static void Color16DitherYV12Mod1X( unsigned char *lum, unsigned char *cr,
                                    unsigned char *cb, unsigned char *out,
                                    int rows, int cols, int mod )
{
    unsigned short* row1;
    unsigned short* row2;
    unsigned char* lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    row1 = (unsigned short*) out;
    row2 = row1 + cols + mod;
    lum2 = lum + cols;

    mod += cols + mod;

    y = rows / 2;
    while( y-- )
    {
        x = cols_2;
        while( x-- )
        {
            register int L;

            cr_r   = 0*768+256 + colortab[ *cr + 0*256 ];
            crb_g  = 1*768+256 + colortab[ *cr + 1*256 ]
                               + colortab[ *cb + 2*256 ];
            cb_b   = 2*768+256 + colortab[ *cb + 3*256 ];
            ++cr; ++cb;

            L = *lum++;
            *row1++ = (unsigned short)(rgb_2_pix[ L + cr_r ] |
                                       rgb_2_pix[ L + crb_g ] |
                                       rgb_2_pix[ L + cb_b ]);

            L = *lum++;
            *row1++ = (unsigned short)(rgb_2_pix[ L + cr_r ] |
                                       rgb_2_pix[ L + crb_g ] |
                                       rgb_2_pix[ L + cb_b ]);


            /* Now, do second row.  */

            L = *lum2++;
            *row2++ = (unsigned short)(rgb_2_pix[ L + cr_r ] |
                                       rgb_2_pix[ L + crb_g ] |
                                       rgb_2_pix[ L + cb_b ]);

            L = *lum2++;
            *row2++ = (unsigned short)(rgb_2_pix[ L + cr_r ] |
                                       rgb_2_pix[ L + crb_g ] |
                                       rgb_2_pix[ L + cb_b ]);
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum  += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}

static void Color32DitherYV12Mod1X( unsigned char *lum, unsigned char *cr,
                                    unsigned char *cb, unsigned char *out,
                                    int rows, int cols, int mod )
{
    unsigned int* row1;
    unsigned int* row2;
    unsigned char* lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    row1 = (unsigned int*) out;
    row2 = row1 + cols + mod;
    lum2 = lum + cols;

    mod += cols + mod;

    y = rows / 2;
    while( y-- )
    {
        x = cols_2;
        while( x-- )
        {
            register int L;

            cr_r   = 0*768+256 + colortab[ *cr + 0*256 ];
            crb_g  = 1*768+256 + colortab[ *cr + 1*256 ]
                               + colortab[ *cb + 2*256 ];
            cb_b   = 2*768+256 + colortab[ *cb + 3*256 ];
            ++cr; ++cb;

            L = *lum++;
            *row1++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);

            L = *lum++;
            *row1++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);


            /* Now, do second row.  */

            L = *lum2++;
            *row2++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);

            L = *lum2++;
            *row2++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum  += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}

void yuv_to_rgb(yuv_frame *in, s_screen *out)
{
    assert(yuv_initialized);
    assert(bytes_per_pixel == pixelbytes[out->pixelformat]);
    void (*convert)(unsigned char *lum, unsigned char *cr,
                    unsigned char *cb, unsigned char *out,
                    int rows, int cols, int mod);
    convert = (bytes_per_pixel == 2) ? Color16DitherYV12Mod1X : Color32DitherYV12Mod1X;
#if REVERSE_COLOR
    convert(in->lum, in->cb, in->cr, out->data, out->height, out->width, 0);
#else
    convert(in->lum, in->cr, in->cb, out->data, out->height, out->width, 0);
#endif
}

