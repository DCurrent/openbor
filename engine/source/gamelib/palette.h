/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef PALETTE_H
#define PALETTE_H

#include "globals.h"

#define		gammacorrect(v,g)	(g<=0?((v*(65025+((255-v)*g)))/65025):(255-(((255-v)*(65025+(v*-g)))/65025)))
#define		brightnesscorrect(v,b)	(b<0?((v*(255+b))/255):(b+((v*(255-b))/255)))
#define		gbcorrect(vx,gx,bx)	(gammacorrect(brightnesscorrect(vx,bx),gx))
// Set gamma/brightness corrected palette.
// Valid values range between -255 and 255, where 0 is normal.
void palette_set_corrected(unsigned char *pal, int gr, int gg, int gb, int br, int bg, int bb);

// Find colour in palette
int palette_find(unsigned char *pal, int r, int g, int b);

typedef unsigned char* (*palette_table_function)(unsigned char*);

// Create lookup tables
unsigned char * palette_table_multiply(unsigned char *pal);
unsigned char * palette_table_screen(unsigned char *pal);
unsigned char * palette_table_dodge(unsigned char *pal);
unsigned char * palette_table_half(unsigned char *pal);
unsigned char * palette_table_overlay(unsigned char *pal);
unsigned char * palette_table_hardlight(unsigned char *pal);


#endif


