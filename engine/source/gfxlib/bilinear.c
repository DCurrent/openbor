// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2004 Forgotten and the VBA development team

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

/**     Code adapted from Exult source code by Forgotten
 **	Scale.cc - Trying to scale with bilinear interpolation.
 **
 **	Written: 6/14/00 - JSF
 **/

/**     Code adapted To OpenBOR by SX
 **	bilinear.c - Trying to scale with bilinear interpolation.
 **
 **	Updated: 5/05/08 - SX
 **/

#include <stdlib.h>
#include "gfxtypes.h"

// 480 == Source Width Max
//   2 == Additional Pixels for Shifting/Blending
//   3 == R,G,B Multiplier

static u8 row_cur[(480+2)*3];  
static u8 row_next[(480+2)*3];

static u8 *rgb_row_cur = row_cur;
static u8 *rgb_row_next = row_next;

#define RGB(r,g,b) ((r)>>3) << GfxRedShift |\
  ((g) >> 3) << GfxGreenShift |\
  ((b) >> 3) << GfxBlueShift\

static void fill_rgb_row_16(u16 *from, int src_width, u8 *row, int width)
{
	u8 *p = NULL;
    u8 *copy_start = row + src_width*3;
	u8 *all_stop = row + width*3;
	while (row < copy_start) 
	{
		u16 color = *from++;
		*row++ = ((color >> GfxRedShift) & 0x1f) << 3;
		*row++ = ((color >> GfxGreenShift) & 0x1f) << 3;
		*row++ = ((color >> GfxBlueShift) & 0x1f) << 3;
	}
	// any remaining elements to be written to 'row' are a replica of the
	// preceding pixel
	p = row-3;
	while (row < all_stop) 
	{
		// we're guaranteed three elements per pixel; could unroll the loop
		// further, especially with a Duff's Device, but the gains would be
		// probably limited (judging by profiler output)
		*row++ = *p++;
		*row++ = *p++;
		*row++ = *p++;
	}
}

static void fill_rgb_row_32(u32 *from, int src_width, u8 *row, int width)
{
	u8 *p = NULL;
	u8 *copy_start = row + src_width*3;
	u8 *all_stop = row + width*3;
	while (row < copy_start) 
	{
		u32 color = *from++;
		*row++ = ((color >> GfxRedShift) & 0x1f) << 3;
		*row++ = ((color >> GfxGreenShift) & 0x1f) << 3;
		*row++ = ((color >> GfxBlueShift) & 0x1f) << 3;
	}
	// any remaining elements to be written to 'row' are a replica of the
	// preceding pixel
	p = row-3;
	while (row < all_stop) 
	{
		// we're guaranteed three elements per pixel; could unroll the loop
		// further, especially with a Duff's Device, but the gains would be
		// probably limited (judging by profiler output)
		*row++ = *p++;
		*row++ = *p++;
		*row++ = *p++;
	}
}

void Bilinear(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	int x, y, from_width = width + 1;
	u8 *temp = NULL;
	u16 *to = (u16 *)dstPtr;
	u16 *to_odd = (u16 *)(dstPtr + dstPitch);
	u16 *from = (u16 *)srcPtr;

	fill_rgb_row_16(from, from_width, rgb_row_cur, width);

	for(y = 0; y < height; y++) 
	{
		u16 *from_orig = from;
		u16 *to_orig = to;
		u8 *cur_row  = NULL;
		u8 *next_row = NULL;
		u8 *ar = NULL;
		u8 *ag = NULL;
		u8 *ab = NULL;
		u8 *cr = NULL;
		u8 *cg = NULL;
		u8 *cb = NULL;
    
		fill_rgb_row_16(from, from_width, rgb_row_next, width);
    
		// every pixel in the src region, is extended to 4 pixels in the
		// destination, arranged in a square 'quad'; if the current src
		// pixel is 'a', then in what follows 'b' is the src pixel to the
		// right, 'c' is the src pixel below, and 'd' is the src pixel to
		// the right and down
		cur_row  = rgb_row_cur;
		next_row = rgb_row_next;
		ar = cur_row++;
		ag = cur_row++;
		ab = cur_row++;
		cr = next_row++;
		cg = next_row++;
		cb = next_row++;
		for(x=0; x < width; x++) 
		{
			u8 *br = cur_row++;
			u8 *bg = cur_row++;
			u8 *bb = cur_row++;
			u8 *dr = next_row++;
			u8 *dg = next_row++;
			u8 *db = next_row++;

		    // upper left pixel in quad: just copy it in
			*to++ = RGB(*ar, *ag, *ab);
      
			// upper right
			*to++ = RGB((*ar+*br)>>1, (*ag+*bg)>>1, (*ab+*bb)>>1);
      
			// lower left
			*to_odd++ = RGB((*ar+*cr)>>1, (*ag+*cg)>>1, (*ab+*cb)>>1);
      
			// lower right
			*to_odd++ = RGB((*ar+*br+*cr+*dr)>>2, (*ag+*bg+*cg+*dg)>>2, (*ab+*bb+*cb+*db)>>2);
      
			// 'b' becomes 'a', 'd' becomes 'c'
			ar = br;
			ag = bg;
			ab = bb;
			cr = dr;
			cg = dg;
			cb = db;
		}
    
		// the "next" rgb row becomes the current; the old current rgb row is
		// recycled and serves as the new "next" row
		temp = rgb_row_cur;
		rgb_row_cur = rgb_row_next;
		rgb_row_next = temp;
    
		// update the pointers for start of next pair of lines
		from = (u16 *)((u8 *)from_orig + srcPitch);
		to = (u16 *)((u8 *)to_orig + (dstPitch << 1));
		to_odd = (u16 *)((u8 *)to + dstPitch);
	}
}

void BilinearPlus(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	int x, y, from_width = width + 1;
	u8 *temp = NULL;
	u16 *to = (u16 *)dstPtr;
	u16 *to_odd = (u16 *)(dstPtr + dstPitch);
	u16 *from = (u16 *)srcPtr;

	fill_rgb_row_16(from, from_width, rgb_row_cur, width);

	for(y = 0; y < height; y++) 
	{
		u16 *from_orig = from;
		u16 *to_orig = to;
		u8 *cur_row  = NULL;
		u8 *next_row = NULL;
		u8 *ar = NULL;
		u8 *ag = NULL;
		u8 *ab = NULL;
		u8 *cr = NULL;
		u8 *cg = NULL;
		u8 *cb = NULL;
    
		fill_rgb_row_16(from, from_width, rgb_row_next, width);
    
	    // every pixel in the src region, is extended to 4 pixels in the
	    // destination, arranged in a square 'quad'; if the current src
		// pixel is 'a', then in what follows 'b' is the src pixel to the
		// right, 'c' is the src pixel below, and 'd' is the src pixel to
		// the right and down
		cur_row  = rgb_row_cur;
		next_row = rgb_row_next;
		ar = cur_row++;
		ag = cur_row++;
		ab = cur_row++;
		cr = next_row++;
		cg = next_row++;
		cb = next_row++;
		for(x=0; x < width; x++) 
		{
			u8 *br = cur_row++;
			u8 *bg = cur_row++;
			u8 *bb = cur_row++;
			u8 *dr = next_row++;
			u8 *dg = next_row++;
			u8 *db = next_row++;
      
		    // upper left pixel in quad: just copy it in
			//*to++ = manip.rgb(*ar, *ag, *ab);

#ifdef USE_ORIGINAL_BILINEAR_PLUS
			*to++ = RGB(
						(((*ar)<<2) +((*ar)) + (*cr+*br+*br) )>> 3,
						(((*ag)<<2) +((*ag)) + (*cg+*bg+*bg) )>> 3,
						(((*ab)<<2) +((*ab)) + (*cb+*bb+*bb) )>> 3);
#else
			*to++ = RGB(
						(((*ar)<<3) +((*ar)<<1) + (*cr+*br+*br+*cr) )>> 4,
						(((*ag)<<3) +((*ag)<<1) + (*cg+*bg+*bg+*cg) )>> 4,
						(((*ab)<<3) +((*ab)<<1) + (*cb+*bb+*bb+*cb) )>> 4);
#endif
      
			// upper right
			*to++ = RGB((*ar+*br)>>1, (*ag+*bg)>>1, (*ab+*bb)>>1);
      
			// lower left
			*to_odd++ = RGB((*ar+*cr)>>1, (*ag+*cg)>>1, (*ab+*cb)>>1);
      
			// lower right
			*to_odd++ = RGB((*ar+*br+*cr+*dr)>>2, (*ag+*bg+*cg+*dg)>>2, (*ab+*bb+*cb+*db)>>2);
      
			// 'b' becomes 'a', 'd' becomes 'c'
			ar = br;
			ag = bg;
			ab = bb;
			cr = dr;
			cg = dg;
			cb = db;
		}
    
		// the "next" rgb row becomes the current; the old current rgb row is
		// recycled and serves as the new "next" row
		temp = rgb_row_cur;
		rgb_row_cur = rgb_row_next;
		rgb_row_next = temp;
    
		// update the pointers for start of next pair of lines
		from = (u16 *)((u8 *)from_orig + srcPitch);
		to = (u16 *)((u8 *)to_orig + (dstPitch << 1));
		to_odd = (u16 *)((u8 *)to + dstPitch);
	}
}

void Bilinear32(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	int x, y, from_width = width + 1;
	u8 *temp = NULL;
	u32 *to = (u32 *)dstPtr;
	u32 *to_odd = (u32 *)(dstPtr + dstPitch);
	u32 *from = (u32 *)srcPtr;

	fill_rgb_row_32(from, from_width, rgb_row_cur, width);

	for(y = 0; y < height; y++) 
	{
		u32 *from_orig = from;
		u32 *to_orig = to;
		u8 *cur_row  = NULL;
		u8 *next_row = NULL;
		u8 *ar = NULL;
		u8 *ag = NULL;
		u8 *ab = NULL;
		u8 *cr = NULL;
		u8 *cg = NULL;
		u8 *cb = NULL;
		
		fill_rgb_row_32(from, from_width, rgb_row_next, width);
    
	    // every pixel in the src region, is extended to 4 pixels in the
		// destination, arranged in a square 'quad'; if the current src
		// pixel is 'a', then in what follows 'b' is the src pixel to the
		// right, 'c' is the src pixel below, and 'd' is the src pixel to
		// the right and down

		cur_row  = rgb_row_cur;
		next_row = rgb_row_next;
		ar = cur_row++;
		ag = cur_row++;
		ab = cur_row++;
		cr = next_row++;
		cg = next_row++;
		cb = next_row++;
		for(x=0; x < width; x++) 
		{
			u8 *br = cur_row++;
			u8 *bg = cur_row++;
			u8 *bb = cur_row++;
			u8 *dr = next_row++;
			u8 *dg = next_row++;
			u8 *db = next_row++;

			// upper left pixel in quad: just copy it in
			*to++ = RGB(*ar, *ag, *ab);
      
			// upper right
			*to++ = RGB((*ar+*br)>>1, (*ag+*bg)>>1, (*ab+*bb)>>1);
      
			// lower left
			*to_odd++ = RGB((*ar+*cr)>>1, (*ag+*cg)>>1, (*ab+*cb)>>1);
      
			// lower right
			*to_odd++ = RGB((*ar+*br+*cr+*dr)>>2, (*ag+*bg+*cg+*dg)>>2, (*ab+*bb+*cb+*db)>>2);
      
			// 'b' becomes 'a', 'd' becomes 'c'
			ar = br;
			ag = bg;
			ab = bb;
			cr = dr;
			cg = dg;
			cb = db;
		}
    
		// the "next" rgb row becomes the current; the old current rgb row is
		// recycled and serves as the new "next" row
		temp = rgb_row_cur;
		rgb_row_cur = rgb_row_next;
		rgb_row_next = temp;
    
		// update the pointers for start of next pair of lines
		from = (u32 *)((u8 *)from_orig + srcPitch);
		to = (u32 *)((u8 *)to_orig + (dstPitch << 1));
		to_odd = (u32 *)((u8 *)to + dstPitch);
	}
}

void BilinearPlus32(u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
	int x, y, from_width = width + 1;
	u8 *temp = NULL;
	u32 *to = (u32 *)dstPtr;
	u32 *to_odd = (u32 *)(dstPtr + dstPitch);
	u32 *from = (u32 *)srcPtr;

	fill_rgb_row_32(from, from_width, rgb_row_cur, width);

	for(y = 0; y < height; y++) 
	{
		u32 *from_orig = from;
		u32 *to_orig = to;
		u8 *cur_row  = NULL;
		u8 *next_row = NULL;
		u8 *ar = NULL;
		u8 *ag = NULL;
		u8 *ab = NULL;
		u8 *cr = NULL;
		u8 *cg = NULL;
		u8 *cb = NULL;
    
		fill_rgb_row_32(from, from_width, rgb_row_next, width);
    
		// every pixel in the src region, is extended to 4 pixels in the
		// destination, arranged in a square 'quad'; if the current src
		// pixel is 'a', then in what follows 'b' is the src pixel to the
		// right, 'c' is the src pixel below, and 'd' is the src pixel to
		// the right and down
		cur_row  = rgb_row_cur;
		next_row = rgb_row_next;
		ar = cur_row++;
		ag = cur_row++;
		ab = cur_row++;
		cr = next_row++;
		cg = next_row++;
		cb = next_row++;
		for(x=0; x < width; x++) 
		{
			u8 *br = cur_row++;
			u8 *bg = cur_row++;
			u8 *bb = cur_row++;
			u8 *dr = next_row++;
			u8 *dg = next_row++;
			u8 *db = next_row++;
      
			// upper left pixel in quad: just copy it in
			//*to++ = manip.rgb(*ar, *ag, *ab);
#ifdef USE_ORIGINAL_BILINEAR_PLUS
			*to++ = RGB(
						(((*ar)<<2) +((*ar)) + (*cr+*br+*br) )>> 3,
						(((*ag)<<2) +((*ag)) + (*cg+*bg+*bg) )>> 3,
						(((*ab)<<2) +((*ab)) + (*cb+*bb+*bb) )>> 3);
#else
			*to++ = RGB(
						(((*ar)<<3) +((*ar)<<1) + (*cr+*br+*br+*cr) )>> 4,
						(((*ag)<<3) +((*ag)<<1) + (*cg+*bg+*bg+*cg) )>> 4,
						(((*ab)<<3) +((*ab)<<1) + (*cb+*bb+*bb+*cb) )>> 4);
#endif
      
			// upper right
			*to++ = RGB((*ar+*br)>>1, (*ag+*bg)>>1, (*ab+*bb)>>1);
      
			// lower left
			*to_odd++ = RGB((*ar+*cr)>>1, (*ag+*cg)>>1, (*ab+*cb)>>1);
      
			// lower right
			*to_odd++ = RGB((*ar+*br+*cr+*dr)>>2, (*ag+*bg+*cg+*dg)>>2, (*ab+*bb+*cb+*db)>>2);
      
			// 'b' becomes 'a', 'd' becomes 'c'
			ar = br;
			ag = bg;
			ab = bb;
			cr = dr;
			cg = dg;
			cb = db;
		}
    
		// the "next" rgb row becomes the current; the old current rgb row is
		// recycled and serves as the new "next" row
		temp = rgb_row_cur;
		rgb_row_cur = rgb_row_next;
		rgb_row_next = temp;
    
		// update the pointers for start of next pair of lines
		from = (u32 *)((u8 *)from_orig + srcPitch);
		to = (u32 *)((u8 *)to_orig + (dstPitch << 1));
		to_odd = (u32 *)((u8 *)to + dstPitch);
	}
}
