/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#ifndef YUV_H
#define YUV_H

// Intializes a context for YUV->RGB color conversion; bits should be 16 or 32
void yuv_init(int bits);

// Performs YUV->RGB color conversion on a single frame
void yuv_to_rgb(unsigned char *lum, unsigned char *cr,
                    unsigned char *cb, unsigned char *out,
                    int rows, int cols, int mod);

// Frees any memory allocated for color conversion
void yuv_clear(void);

#endif
