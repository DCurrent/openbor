// snes9x/gfx.h

/*
 * Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 *
 * (c) Copyright 1996 - 2001 Gary Henderson (gary.henderson@ntlworld.com) and
 *                           Jerremy Koot (jkoot@snes9x.com)
 *
 * Super FX C emulator code 
 * (c) Copyright 1997 - 1999 Ivar (ivar@snes9x.com) and
 *                           Gary Henderson.
 * Super FX assembler emulator code (c) Copyright 1998 zsKnight and _Demo_.
 *
 * DSP1 emulator code (c) Copyright 1998 Ivar, _Demo_ and Gary Henderson.
 * C4 asm and some C emulation code (c) Copyright 2000 zsKnight and _Demo_.
 * C4 C code (c) Copyright 2001 Gary Henderson (gary.henderson@ntlworld.com).
 *
 * DOS port code contains the works of other authors. See headers in
 * individual files.
 *
 * Snes9x homepage: http://www.snes9x.com
 *
 * Permission to use, copy, modify and distribute Snes9x in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Snes9x is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Snes9x or software derived from Snes9x.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so everyone can benefit from the modifications
 * in future versions.
 *
 * Super NES and Super Nintendo Entertainment System are trademarks of
 * Nintendo Co., Limited and its subsidiary companies.
 */
#ifndef _GFX_H_
#define _GFX_H_

typedef unsigned char bool8;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef signed char int8;
typedef short int16;
typedef int int32;
typedef unsigned int uint32;
typedef long int64;

extern uint32 odd_high [4][16];
extern uint32 odd_low [4][16];
extern uint32 even_high [4][16];
extern uint32 even_low [4][16];
extern uint16 DirectColourMaps [8][256];

extern uint8 add32_32 [32][32];
extern uint8 add32_32_half [32][32];
extern uint8 sub32_32 [32][32];
extern uint8 sub32_32_half [32][32];
extern uint8 mul_brightness [16][32];

// Could use BSWAP instruction on Intel port...
#define SWAP_DWORD(dw) dw = ((dw & 0xff) << 24) | ((dw & 0xff00) << 8) | \
		            ((dw & 0xff0000) >> 8) | ((dw & 0xff000000) >> 24)

#ifdef FAST_LSB_WORD_ACCESS
#define READ_2BYTES(s) (*(uint16 *) (s))
#define WRITE_2BYTES(s, d) *(uint16 *) (s) = (d)
#else
#ifdef LSB_FIRST
#define READ_2BYTES(s) (*(uint8 *) (s) | (*((uint8 *) (s) + 1) << 8))
#define WRITE_2BYTES(s, d) *(uint8 *) (s) = (d), \
			   *((uint8 *) (s) + 1) = (d) >> 8
#else  // else MSB_FISRT
#define READ_2BYTES(s) (*(uint8 *) (s) | (*((uint8 *) (s) + 1) << 8))
#define WRITE_2BYTES(s, d) *(uint8 *) (s) = (d), \
			   *((uint8 *) (s) + 1) = (d) >> 8
#endif // LSB_FIRST
#endif // i386

#define SUB_SCREEN_DEPTH 0
#define MAIN_SCREEN_DEPTH 32

#if defined(OLD_COLOUR_BLENDING)
#define COLOR_ADD(C1, C2) \
GFX.X2 [((((C1) & RGB_REMOVE_LOW_BITS_MASK) + \
	  ((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1) + \
	((C1) & (C2) & RGB_LOW_BITS_MASK)]
#else
#define COLOR_ADD(C1, C2) \
(GFX.X2 [((((C1) & RGB_REMOVE_LOW_BITS_MASK) + \
	  ((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1) + \
	 ((C1) & (C2) & RGB_LOW_BITS_MASK)] | \
 (((C1) ^ (C2)) & RGB_LOW_BITS_MASK))	   
#endif

#define COLOR_ADD1_2(C1, C2) \
(((((C1) & RGB_REMOVE_LOW_BITS_MASK) + \
          ((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1) + \
         ((C1) & (C2) & RGB_LOW_BITS_MASK) | ALPHA_BITS_MASK)

#if defined(OLD_COLOUR_BLENDING)
#define COLOR_SUB(C1, C2) \
GFX.ZERO_OR_X2 [(((C1) | RGB_HI_BITS_MASKx2) - \
		 ((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1]
#else
#define COLOR_SUB(C1, C2) \
(GFX.ZERO_OR_X2 [(((C1) | RGB_HI_BITS_MASKx2) - \
                  ((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1] + \
((C1) & RGB_LOW_BITS_MASK) - ((C2) & RGB_LOW_BITS_MASK))
#endif

#define COLOR_SUB1_2(C1, C2) \
GFX.ZERO [(((C1) | RGB_HI_BITS_MASKx2) - \
	   ((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1]

typedef void (*NormalTileRenderer) (uint32 Tile, uint32 Offset, 
				    uint32 StartLine, uint32 LineCount);
typedef void (*ClippedTileRenderer) (uint32 Tile, uint32 Offset,
				     uint32 StartPixel, uint32 Width,
				     uint32 StartLine, uint32 LineCount);
typedef void (*LargePixelRenderer) (uint32 Tile, uint32 Offset,
				    uint32 StartPixel, uint32 Pixels,
				    uint32 StartLine, uint32 LineCount);

#ifdef IMACPPBITCH
extern void _2xSaI(uint8 *srcPtr, uint32 srcPitch,
		 uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
extern void SuperEagle(uint8 *srcPtr, uint32 srcPitch,
		 uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
extern void Super2xSaI(uint8 *srcPtr, uint32 srcPitch,
		 uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
extern void Scale2x(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
extern void SuperScale(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
extern void Eagle(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);

#else
void _2xSaI(uint8 *srcPtr, uint32 srcPitch,
		 uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void SuperEagle(uint8 *srcPtr, uint32 srcPitch,
		uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void Super2xSaI(uint8 *srcPtr, uint32 srcPitch,
		 uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void Scale2x(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void SuperScale(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void Eagle(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
#endif


//extern uint16	*asmsai_srcPtr;
uint16	*asmsai_srcPtr;
uint16	*asmsai_deltaPtr;
uint32	asmsai_srcPitch;
uint32	asmsai_width;    
uint32	*asmsai_dstOffset;  
uint32	asmsai_dstPitch;     
uint8	*asmsai_dstSegment; //FOR ASM OPTI!!!

#endif
