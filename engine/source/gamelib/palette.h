/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef PALETTE_H
#define PALETTE_H

#include "globals.h"

// Find colour in palette
int palette_find(unsigned char *pal, int r, int g, int b);

typedef unsigned char *(*palette_table_function)(unsigned char *);
typedef unsigned char *(*blend_table_function)(void);

// Create lookup tables
unsigned char *palette_table_multiply(unsigned char *pal);
unsigned char *palette_table_screen(unsigned char *pal);
unsigned char *palette_table_dodge(unsigned char *pal);
unsigned char *palette_table_half(unsigned char *pal);
unsigned char *palette_table_overlay(unsigned char *pal);
unsigned char *palette_table_hardlight(unsigned char *pal);

// these are in pixelformat.c, technologically they are not palette related
// but move them here since they share similar logic
unsigned char *create_multiply32_tbl();
unsigned char *create_screen32_tbl();
unsigned char *create_dodge32_tbl();
unsigned char *create_half32_tbl();
unsigned char *create_overlay32_tbl();
unsigned char *create_hardlight32_tbl();

#endif


