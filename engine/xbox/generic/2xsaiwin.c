// snes9x/2xsaiwin.cpp

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

#include "2xsaiwin.h"
#include "stdio.h"

#define MMX

#define ASM_JUMP_ALIGN ".p2align 4\n"

#define assert_align(x) \
	do { } while (0)


#define bool	int
#define false	0
#define true	1

#ifdef MMX
//void Init_2xSaIMMX(BitFormat);
void _2xSaILineW(uint8 *srcPtr, uint8 *deltaPtr, uint32 srcPitch, uint32 width,
						uint8 *dstPtr, uint32 dstPitch);
void _2xSaISuperEagleLineW(uint8 *srcPtr, uint8 *deltaPtr, uint32 srcPitch, uint32 width,
						uint8 *dstPtr, uint32 dstPitch);
void _2xSaISuper2xSaILineW(uint8 *srcPtr, uint8 *deltaPtr, uint32 srcPitch, uint32 width,
						uint8 *dstPtr, uint32 dstPitch);
void _2xSaISuperScaleLineW(uint16 *src0, uint16 *src1, uint16 *src2, uint16 *dst, uint32 width);
void _2xSaISuperScale75LineW(uint16 *src0, uint16 *src1, uint16 *src2, uint16 *dst, uint32 width, unsigned __int64 *mask);
void _2xSaIEagleLineW(uint8 *src0, uint8 *src1, uint32 width, uint16 *dst0, uint16 *dst1);
int Init_2xSaIMMXW(uint32 BitFormat);
#endif

bool mmx_cpu = false;

static uint32 colorMask = 0xF7DEF7DE;
static uint32 lowPixelMask = 0x08210821;
static uint32 qcolorMask = 0xE79CE79C;
static uint32 qlowpixelMask = 0x18631863;

int Init_2xSaI(uint32 BitFormat)
{
		if (BitFormat == 565)
		{
				colorMask = 0xF7DEF7DE;
				lowPixelMask = 0x08210821;
				qcolorMask = 0xE79CE79C;
				qlowpixelMask = 0x18631863;
		}
		else
		if (BitFormat == 555)
		{
				colorMask = 0x7BDE7BDE;
				lowPixelMask = 0x04210421;
				qcolorMask = 0x739C739C;
				qlowpixelMask = 0x0C630C63;
		}
		else
		{
				return 0;
		}
#ifdef MMX
		Init_2xSaIMMXW(BitFormat);
#endif
		return 1;
}

static _inline int GetResult1(uint32 A, uint32 B, uint32 C, uint32 D, uint32 E)
{
 int x = 0;
 int y = 0;
 int r = 0;
 if (A == C) x+=1; else if (B == C) y+=1;
 if (A == D) x+=1; else if (B == D) y+=1;
 if (x <= 1) r+=1;
 if (y <= 1) r-=1;
 return r;
}

static _inline int GetResult2(uint32 A, uint32 B, uint32 C, uint32 D, uint32 E)
{
 int x = 0;
 int y = 0;
 int r = 0;
 if (A == C) x+=1; else if (B == C) y+=1;
 if (A == D) x+=1; else if (B == D) y+=1;
 if (x <= 1) r-=1;
 if (y <= 1) r+=1;
 return r;
}


static _inline int GetResult(uint32 A, uint32 B, uint32 C, uint32 D)
{
 int x = 0;
 int y = 0;
 int r = 0;
 if (A == C) x+=1; else if (B == C) y+=1;
 if (A == D) x+=1; else if (B == D) y+=1;
 if (x <= 1) r+=1;
 if (y <= 1) r-=1;
 return r;
}


static _inline uint32 INTERPOLATE(uint32 A, uint32 B)
{
	if (A !=B)
	{
	   return ( ((A & colorMask) >> 1) + ((B & colorMask) >> 1) + (A & B & lowPixelMask) );
	}
	else return A;
}


static _inline uint32 Q_INTERPOLATE(uint32 A, uint32 B, uint32 C, uint32 D)
{
		register uint32 x = ((A & qcolorMask) >> 2) +
							((B & qcolorMask) >> 2) +
							((C & qcolorMask) >> 2) +
							((D & qcolorMask) >> 2);
		register uint32 y = (A & qlowpixelMask) +
							(B & qlowpixelMask) +
							(C & qlowpixelMask) +
							(D & qlowpixelMask);
		y = (y>>2) & qlowpixelMask;
		return x+y;
}




#define HOR
#define VER
extern void Super2xSaI(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode)
{
	uint32 *dP;
	uint16 *bP;
	uint16 *xP;
#ifdef MMX
	if (true)
	{
	for (height; height; height-=1)
	{
	    	bP = (uint16 *) srcPtr;
			xP = (uint16 *) deltaPtr;
	    	dP = (uint32 *) dstPtr;
			_2xSaISuper2xSaILineW((uint8 *) bP, (uint8 *) xP, srcPitch, width, (uint8 *) dP, dstPitch);
		dstPtr += dstPitch << 1;
	    	srcPtr += srcPitch;
			deltaPtr += srcPitch;
		}
	}
	else
	{
#endif
		uint32 Nextline = srcPitch >> 1;

		for (height; height; height-=1)
	{
	    uint32 finish;

	    bP = (uint16 *) srcPtr;
	    dP = (uint32 *) dstPtr;
			for (finish = width; finish; finish -= 1 )
			{
		   	uint32 color4, color5, color6;
		   	uint32 color1, color2, color3;
		   	uint32 colorA0, colorA1, colorA2, colorA3,
				  	colorB0, colorB1, colorB2, colorB3,
				  	colorS1, colorS2;
		   	uint32 product1a, product1b,
				  	product2a, product2b;

//---------------------------------------    B1 B2
//                                         4  5  6 S2
//                                         1  2  3 S1
//                                           A1 A2

				colorB0 = *(bP- Nextline - 1);
				colorB1 = *(bP- Nextline);
				colorB2 = *(bP- Nextline + 1);
				colorB3 = *(bP- Nextline + 2);

				color4 = *(bP - 1);
				color5 = *(bP);
				color6 = *(bP + 1);
				colorS2 = *(bP + 2);

				color1 = *(bP + Nextline - 1);
				color2 = *(bP + Nextline);
				color3 = *(bP + Nextline + 1);
				colorS1 = *(bP + Nextline + 2);

				colorA0 = *(bP + Nextline + Nextline - 1);
				colorA1 = *(bP + Nextline + Nextline);
				colorA2 = *(bP + Nextline + Nextline + 1);
				colorA3 = *(bP + Nextline + Nextline + 2);


//--------------------------------------
				if (color2 == color6 && color5 != color3)
				{
				   product2b = product1b = color2;
				}
				else
				if (color5 == color3 && color2 != color6)
				{
				   product2b = product1b = color5;
				}
				else
				if (color5 == color3 && color2 == color6 && color5 != color6)
				{
				   register int r = 0;

				   r += GetResult (color6, color5, color1, colorA1);
				   r += GetResult (color6, color5, color4, colorB1);
				   r += GetResult (color6, color5, colorA2, colorS1);
				   r += GetResult (color6, color5, colorB2, colorS2);

				   if (r > 0)
					  product2b = product1b = color6;
				   else
				   if (r < 0)
					  product2b = product1b = color5;
				   else
				   {
					  product2b = product1b = INTERPOLATE (color5, color6);
				   }

				}
				else
				{

#ifdef VER
				   if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
					  product2b = Q_INTERPOLATE (color3, color3, color3, color2);
				   else
				   if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
					  product2b = Q_INTERPOLATE (color2, color2, color2, color3);
				   else
#endif
					  product2b = INTERPOLATE (color2, color3);

#ifdef VER
				   if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
					  product1b = Q_INTERPOLATE (color6, color6, color6, color5);
				   else
				   if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
					  product1b = Q_INTERPOLATE (color6, color5, color5, color5);
				   else
#endif
					  product1b = INTERPOLATE (color5, color6);
				}

#ifdef HOR
				if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
				   product2a = INTERPOLATE (color2, color5);
				else
				if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
				   product2a = INTERPOLATE(color2, color5);
				else
#endif
				   product2a = color2;

#ifdef HOR
				if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
				   product1a = INTERPOLATE (color2, color5);
				else
				if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
				   product1a = INTERPOLATE(color2, color5);
				else
#endif
				   product1a = color5;


				product1a = product1a | (product1b << 16);
				product2a = product2a | (product2b << 16);

		*(dP) = product1a;
		*(dP+(dstPitch>>2)) = product2a;

				bP += 1;
				dP += 1;
			}//end of for ( finish= width etc..)

	    dstPtr += dstPitch << 1;
			srcPtr += srcPitch;
			deltaPtr += srcPitch;
	}; //endof: for (height; height; height--)
#ifdef MMX
	}
#endif
}


extern void Super2xSaIScanline(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode)
{
	uint32 *dP;
	uint16 *bP;
	uint16 *xP;
#ifdef MMX
	if (true)
	{
	for (height; height; height-=1)
	{
	    	bP = (uint16 *) srcPtr;
			xP = (uint16 *) deltaPtr;
	    	dP = (uint32 *) dstPtr;
			_2xSaISuper2xSaILineW((uint8 *) bP, (uint8 *) xP, srcPitch, width, (uint8 *) dP, dstPitch);

		if ( height & 0x01 )
		{
			memset( dstPtr, 0, width<<2 ) ;
			//memset( dstPtr+dstPitch, 0, width<<2 ) ;
			//memset( bP, 0, width<<1 ) ;
			//memset( bP+srcPitch, 0, width<<1 ) ;
		}

		dstPtr += dstPitch << 1;
	    	srcPtr += srcPitch;
			deltaPtr += srcPitch;
		}
	}
	else
	{
#endif
		uint32 Nextline = srcPitch >> 1;

		for (height; height; height-=1)
	{
	    uint32 finish;

	    bP = (uint16 *) srcPtr;
	    dP = (uint32 *) dstPtr;
			for (finish = width; finish; finish -= 1 )
			{
		   	uint32 color4, color5, color6;
		   	uint32 color1, color2, color3;
		   	uint32 colorA0, colorA1, colorA2, colorA3,
				  	colorB0, colorB1, colorB2, colorB3,
				  	colorS1, colorS2;
		   	uint32 product1a, product1b,
				  	product2a, product2b;

//---------------------------------------    B1 B2
//                                         4  5  6 S2
//                                         1  2  3 S1
//                                           A1 A2

				colorB0 = *(bP- Nextline - 1);
				colorB1 = *(bP- Nextline);
				colorB2 = *(bP- Nextline + 1);
				colorB3 = *(bP- Nextline + 2);

				color4 = *(bP - 1);
				color5 = *(bP);
				color6 = *(bP + 1);
				colorS2 = *(bP + 2);

				color1 = *(bP + Nextline - 1);
				color2 = *(bP + Nextline);
				color3 = *(bP + Nextline + 1);
				colorS1 = *(bP + Nextline + 2);

				colorA0 = *(bP + Nextline + Nextline - 1);
				colorA1 = *(bP + Nextline + Nextline);
				colorA2 = *(bP + Nextline + Nextline + 1);
				colorA3 = *(bP + Nextline + Nextline + 2);


//--------------------------------------
				if (color2 == color6 && color5 != color3)
				{
				   product2b = product1b = color2;
				}
				else
				if (color5 == color3 && color2 != color6)
				{
				   product2b = product1b = color5;
				}
				else
				if (color5 == color3 && color2 == color6 && color5 != color6)
				{
				   register int r = 0;

				   r += GetResult (color6, color5, color1, colorA1);
				   r += GetResult (color6, color5, color4, colorB1);
				   r += GetResult (color6, color5, colorA2, colorS1);
				   r += GetResult (color6, color5, colorB2, colorS2);

				   if (r > 0)
					  product2b = product1b = color6;
				   else
				   if (r < 0)
					  product2b = product1b = color5;
				   else
				   {
					  product2b = product1b = INTERPOLATE (color5, color6);
				   }

				}
				else
				{

#ifdef VER
				   if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
					  product2b = Q_INTERPOLATE (color3, color3, color3, color2);
				   else
				   if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
					  product2b = Q_INTERPOLATE (color2, color2, color2, color3);
				   else
#endif
					  product2b = INTERPOLATE (color2, color3);

#ifdef VER
				   if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
					  product1b = Q_INTERPOLATE (color6, color6, color6, color5);
				   else
				   if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
					  product1b = Q_INTERPOLATE (color6, color5, color5, color5);
				   else
#endif
					  product1b = INTERPOLATE (color5, color6);
				}

#ifdef HOR
				if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
				   product2a = INTERPOLATE (color2, color5);
				else
				if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
				   product2a = INTERPOLATE(color2, color5);
				else
#endif
				   product2a = color2;

#ifdef HOR
				if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
				   product1a = INTERPOLATE (color2, color5);
				else
				if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
				   product1a = INTERPOLATE(color2, color5);
				else
#endif
				   product1a = color5;


				product1a = product1a | (product1b << 16);
				product2a = product2a | (product2b << 16);

		*(dP) = product1a;
		*(dP+(dstPitch>>2)) = product2a;

				bP += 1;
				dP += 1;
			}//end of for ( finish= width etc..)

	    dstPtr += dstPitch << 1;
			srcPtr += srcPitch;
			deltaPtr += srcPitch;
	}; //endof: for (height; height; height--)
#ifdef MMX
	}
#endif
}




/*ONLY use with 640x480x16 or higher resolutions*/
/*Only use this if 2*width * 2*height fits on the current screen*/
extern void SuperEagle(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode)
{
	uint32 *dP;
	uint16 *bP;
	uint16 *xP;
#ifdef MMX
//  if (mmx_cpu && width != 512)
	if (true)
	{
	for (height; height; height-=1)
	{
		bP = (uint16 *) srcPtr;
		xP = (uint16 *) deltaPtr;
		dP = (uint32 *) dstPtr;
			_2xSaISuperEagleLineW((uint8 *) bP, (uint8 *) xP, srcPitch, width, (uint8 *)dP, dstPitch);
		dstPtr += dstPitch << 1;
	    	srcPtr += srcPitch;
			deltaPtr += srcPitch;
		}
	}
	else
	{
#endif
		uint32 Nextline = srcPitch >> 1;

		for (height; height; height-=1)
	{
	    uint32 finish;

	    bP = (uint16 *) srcPtr;
	    dP = (uint32 *) dstPtr;
			for (finish = width; finish; finish -= 1 )
			{

		   	uint32 color4, color5, color6;
		   	uint32 color1, color2, color3;
		   	uint32 colorA0, colorA1, colorA2, colorA3,
				  colorB0, colorB1, colorB2, colorB3,
				  colorS1, colorS2;
		   	uint32 product1a, product1b,
				  product2a, product2b;

				colorB0 = *(bP- Nextline - 1);
				colorB1 = *(bP- Nextline);
				colorB2 = *(bP- Nextline + 1);
				colorB3 = *(bP- Nextline + 2);

				color4 = *(bP - 1);
				color5 = *(bP);
				color6 = *(bP + 1);
				colorS2 = *(bP + 2);

				color1 = *(bP + Nextline - 1);
				color2 = *(bP + Nextline);
				color3 = *(bP + Nextline + 1);
				colorS1 = *(bP + Nextline + 2);

				colorA0 = *(bP + Nextline + Nextline - 1);
				colorA1 = *(bP + Nextline + Nextline);
				colorA2 = *(bP + Nextline + Nextline + 1);
				colorA3 = *(bP + Nextline + Nextline + 2);


				//--------------------------------------
				if (color2 == color6 && color5 != color3)
				{
				   product1b = product2a = color2;
				   if ((color1 == color2 && color6 == colorS2) ||
					   (color2 == colorA1 && color6 == colorB2))
				   {
					   product1a = INTERPOLATE (color2, color5);
					   product1a = INTERPOLATE (color2, product1a);
					   product2b = INTERPOLATE (color2, color3);
					   product2b = INTERPOLATE (color2, product2b);
//                       product1a = color2;
//                       product2b = color2;
				   }
				   else
				   {
					  product1a = INTERPOLATE (color5, color6);
					  product2b = INTERPOLATE (color2, color3);
				   }
				}
				else
				if (color5 == color3 && color2 != color6)
				{
				   product2b = product1a = color5;
				   if ((colorB1 == color5 && color3 == colorA2) ||
					   (color4 == color5 && color3 == colorS1))
				   {
					   product1b = INTERPOLATE (color5, color6);
					   product1b = INTERPOLATE (color5, product1b);
					   product2a = INTERPOLATE (color5, color2);
					   product2a = INTERPOLATE (color5, product2a);
//                       product1b = color5;
//                       product2a = color5;
				   }
				   else
				   {
					  product1b = INTERPOLATE (color5, color6);
					  product2a = INTERPOLATE (color2, color3);
				   }
				}
				else
				if (color5 == color3 && color2 == color6 && color5 != color6)
				{
				   register int r = 0;

				   r += GetResult (color6, color5, color1, colorA1);
				   r += GetResult (color6, color5, color4, colorB1);
				   r += GetResult (color6, color5, colorA2, colorS1);
				   r += GetResult (color6, color5, colorB2, colorS2);

				   if (r > 0)
				   {
					  product1b = product2a = color2;
					  product1a = product2b = INTERPOLATE (color5, color6);
				   }
				   else
				   if (r < 0)
				   {
					  product2b = product1a = color5;
					  product1b = product2a = INTERPOLATE (color5, color6);
				   }
				   else
				   {
					  product2b = product1a = color5;
					  product1b = product2a = color2;
				   }
				}
				else
				{

				   if ((color2 == color5) || (color3 == color6))
				   {
					  product1a = color5;
					  product2a = color2;
					  product1b = color6;
					  product2b = color3;

				   }
				   else
				   {
					  product1b = product1a = INTERPOLATE (color5, color6);
					  product1a = INTERPOLATE (color5, product1a);
					  product1b = INTERPOLATE (color6, product1b);

					  product2a = product2b = INTERPOLATE (color2, color3);
					  product2a = INTERPOLATE (color2, product2a);
					  product2b = INTERPOLATE (color3, product2b);
				   }
				}


				product1a = product1a | (product1b << 16);
				product2a = product2a | (product2b << 16);

		*(dP) = product1a;
		*(dP+(dstPitch>>2)) = product2a;

				bP += 1;
				dP += 1;
			}//end of for ( finish= width etc..)

	    dstPtr += dstPitch << 1;
			srcPtr += srcPitch;
			deltaPtr += srcPitch;
	}; //endof: for (height; height; height--)
#ifdef MMX
	}
#endif
}

/*ONLY use with 640x480x16 or higher resolutions*/
/*Only use this if 2*width * 2*height fits on the current screen*/
extern void SuperEagleScanline(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode)
{
	uint32 *dP;
	uint16 *bP;
	uint16 *xP;
#ifdef MMX
//  if (mmx_cpu && width != 512)
	if (true)
	{
	for (height; height; height-=1)
	{
		bP = (uint16 *) srcPtr;
		xP = (uint16 *) deltaPtr;
		dP = (uint32 *) dstPtr;
			_2xSaISuperEagleLineW((uint8 *) bP, (uint8 *) xP, srcPitch, width, (uint8 *)dP, dstPitch);

		if ( height & 0x01 )
		{
			memset( dstPtr, 0, width<<2 ) ;
		}
		dstPtr += dstPitch << 1;
	    	srcPtr += srcPitch;
			deltaPtr += srcPitch;
		}
	}
	else
	{
#endif
		uint32 Nextline = srcPitch >> 1;

		for (height; height; height-=1)
	{
	    uint32 finish;

	    bP = (uint16 *) srcPtr;
	    dP = (uint32 *) dstPtr;
			for (finish = width; finish; finish -= 1 )
			{

		   	uint32 color4, color5, color6;
		   	uint32 color1, color2, color3;
		   	uint32 colorA0, colorA1, colorA2, colorA3,
				  colorB0, colorB1, colorB2, colorB3,
				  colorS1, colorS2;
		   	uint32 product1a, product1b,
				  product2a, product2b;

				colorB0 = *(bP- Nextline - 1);
				colorB1 = *(bP- Nextline);
				colorB2 = *(bP- Nextline + 1);
				colorB3 = *(bP- Nextline + 2);

				color4 = *(bP - 1);
				color5 = *(bP);
				color6 = *(bP + 1);
				colorS2 = *(bP + 2);

				color1 = *(bP + Nextline - 1);
				color2 = *(bP + Nextline);
				color3 = *(bP + Nextline + 1);
				colorS1 = *(bP + Nextline + 2);

				colorA0 = *(bP + Nextline + Nextline - 1);
				colorA1 = *(bP + Nextline + Nextline);
				colorA2 = *(bP + Nextline + Nextline + 1);
				colorA3 = *(bP + Nextline + Nextline + 2);


				//--------------------------------------
				if (color2 == color6 && color5 != color3)
				{
				   product1b = product2a = color2;
				   if ((color1 == color2 && color6 == colorS2) ||
					   (color2 == colorA1 && color6 == colorB2))
				   {
					   product1a = INTERPOLATE (color2, color5);
					   product1a = INTERPOLATE (color2, product1a);
					   product2b = INTERPOLATE (color2, color3);
					   product2b = INTERPOLATE (color2, product2b);
//                       product1a = color2;
//                       product2b = color2;
				   }
				   else
				   {
					  product1a = INTERPOLATE (color5, color6);
					  product2b = INTERPOLATE (color2, color3);
				   }
				}
				else
				if (color5 == color3 && color2 != color6)
				{
				   product2b = product1a = color5;
				   if ((colorB1 == color5 && color3 == colorA2) ||
					   (color4 == color5 && color3 == colorS1))
				   {
					   product1b = INTERPOLATE (color5, color6);
					   product1b = INTERPOLATE (color5, product1b);
					   product2a = INTERPOLATE (color5, color2);
					   product2a = INTERPOLATE (color5, product2a);
//                       product1b = color5;
//                       product2a = color5;
				   }
				   else
				   {
					  product1b = INTERPOLATE (color5, color6);
					  product2a = INTERPOLATE (color2, color3);
				   }
				}
				else
				if (color5 == color3 && color2 == color6 && color5 != color6)
				{
				   register int r = 0;

				   r += GetResult (color6, color5, color1, colorA1);
				   r += GetResult (color6, color5, color4, colorB1);
				   r += GetResult (color6, color5, colorA2, colorS1);
				   r += GetResult (color6, color5, colorB2, colorS2);

				   if (r > 0)
				   {
					  product1b = product2a = color2;
					  product1a = product2b = INTERPOLATE (color5, color6);
				   }
				   else
				   if (r < 0)
				   {
					  product2b = product1a = color5;
					  product1b = product2a = INTERPOLATE (color5, color6);
				   }
				   else
				   {
					  product2b = product1a = color5;
					  product1b = product2a = color2;
				   }
				}
				else
				{

				   if ((color2 == color5) || (color3 == color6))
				   {
					  product1a = color5;
					  product2a = color2;
					  product1b = color6;
					  product2b = color3;

				   }
				   else
				   {
					  product1b = product1a = INTERPOLATE (color5, color6);
					  product1a = INTERPOLATE (color5, product1a);
					  product1b = INTERPOLATE (color6, product1b);

					  product2a = product2b = INTERPOLATE (color2, color3);
					  product2a = INTERPOLATE (color2, product2a);
					  product2b = INTERPOLATE (color3, product2b);
				   }
				}


				product1a = product1a | (product1b << 16);
				product2a = product2a | (product2b << 16);

		*(dP) = product1a;
		*(dP+(dstPitch>>2)) = product2a;

				bP += 1;
				dP += 1;
			}//end of for ( finish= width etc..)

	    dstPtr += dstPitch << 1;
			srcPtr += srcPitch;
			deltaPtr += srcPitch;
	}; //endof: for (height; height; height--)
#ifdef MMX
	}
#endif
}



/*ONLY use with 640x480x16 or higher resolutions*/
/*Only use this if 2*width * 2*height fits on the current screen*/
extern void _2xSaI(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode)
{
	uint32 *dP;
	uint16 *bP;
	uint16 *xP;
#ifdef MMX
//  if (mmx_cpu && width != 512)
	for (height; height; height-=1)
	{

		//sprintfx( "2xsai %u\r\n", height ) ;
	    bP = (uint16 *) srcPtr;
	    xP = (uint16 *) deltaPtr;
	    dP = (uint32 *) dstPtr;
		_2xSaILineW((uint8 *) bP, (uint8 *) xP, srcPitch, width, (uint8 *)dP, dstPitch);
	    dstPtr += dstPitch << 1;
	    srcPtr += srcPitch;
			deltaPtr += srcPitch;
	}
#else
		uint32 Nextline = srcPitch >> 1;

		Init_2xSaI(565);

		for (height; height; height-=1)
		{
	   	    uint32 finish;

	        bP = (uint16 *) srcPtr;
	        dP = (uint32 *) dstPtr;
			for (finish = width; finish; finish -= 1 )
			{
				register uint32 colorA, colorB;
				uint32 colorC, colorD,
					   colorE, colorF, colorG, colorH,
					   colorI, colorJ, colorK, colorL,
					   colorM, colorN, colorO, colorP;
				uint32 product, product1, product2;


//---------------------------------------
// Map of the pixels:                    I|E F|J
//                                       G|A B|K
//                                       H|C D|L
//                                       M|N O|P
				colorI = *(bP- Nextline - 1);
				colorE = *(bP- Nextline);
				colorF = *(bP- Nextline + 1);
				colorJ = *(bP- Nextline + 2);

				colorG = *(bP - 1);
				colorA = *(bP);
				colorB = *(bP + 1);
				colorK = *(bP + 2);

				colorH = *(bP + Nextline - 1);
				colorC = *(bP + Nextline);
				colorD = *(bP + Nextline + 1);
				colorL = *(bP + Nextline + 2);

				colorM = *(bP + Nextline + Nextline - 1);
				colorN = *(bP + Nextline + Nextline);
				colorO = *(bP + Nextline + Nextline + 1);
				colorP = *(bP + Nextline + Nextline + 2);

						if ((colorA == colorD) && (colorB != colorC))
						{
						   if ( ((colorA == colorE) && (colorB == colorL)) ||
								((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ)) )
						   {
							  product = colorA;
						   }
						   else
						   {
							  product = INTERPOLATE(colorA, colorB);
						   }

						   if (((colorA == colorG) && (colorC == colorO)) ||
							   ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM)) )
						   {
							  product1 = colorA;
						   }
						   else
						   {
							  product1 = INTERPOLATE(colorA, colorC);
						   }
						   product2 = colorA;
						}
						else
						if ((colorB == colorC) && (colorA != colorD))
						{
						   if (((colorB == colorF) && (colorA == colorH)) ||
							   ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI)) )
						   {
							  product = colorB;
						   }
						   else
						   {
							  product = INTERPOLATE(colorA, colorB);
						   }

						   if (((colorC == colorH) && (colorA == colorF)) ||
							   ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI)) )
						   {
							  product1 = colorC;
						   }
						   else
						   {
							  product1 = INTERPOLATE(colorA, colorC);
						   }
						   product2 = colorB;
						}
						else
						if ((colorA == colorD) && (colorB == colorC))
						{
						   if (colorA == colorB)
						   {
							  product = colorA;
							  product1 = colorA;
							  product2 = colorA;
						   }
						   else
						   {
							  register int r = 0;
							  product1 = INTERPOLATE(colorA, colorC);
							  product = INTERPOLATE(colorA, colorB);

							  r += GetResult1 (colorA, colorB, colorG, colorE, colorI);
							  r += GetResult2 (colorB, colorA, colorK, colorF, colorJ);
							  r += GetResult2 (colorB, colorA, colorH, colorN, colorM);
							  r += GetResult1 (colorA, colorB, colorL, colorO, colorP);

							  if (r > 0)
								  product2 = colorA;
							  else
							  if (r < 0)
								  product2 = colorB;
							  else
							  {
								  product2 = Q_INTERPOLATE(colorA, colorB, colorC, colorD);
							  }
						   }
						}
						else
						{
						   product2 = Q_INTERPOLATE(colorA, colorB, colorC, colorD);

						   if ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ))
						   {
							  product = colorA;
						   }
						   else
						   if ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI))
						   {
							  product = colorB;
						   }
						   else
						   {
							  product = INTERPOLATE(colorA, colorB);
						   }

						   if ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM))
						   {
							  product1 = colorA;
						   }
						   else
						   if ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI))
						   {
							  product1 = colorC;
						   }
						   else
						   {
							  product1 = INTERPOLATE(colorA, colorC);
						   }
						}
						product = colorA | (product << 16);
						product1 = product1 | (product2 << 16);

			*(dP) = product;
			*(dP+(dstPitch>>2)) = product1;

					bP += 1;
					dP += 1;
				}//end of for ( finish= width etc..)

			dstPtr += dstPitch << 1;
			srcPtr += srcPitch;
			deltaPtr += srcPitch;
	}; //endof: for (height; height; height--)
#endif //MMX
}


/*ONLY use with 640x480x16 or higher resolutions*/
/*Only use this if 2*width * 2*height fits on the current screen*/
extern void _2xSaIScanline(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode)
{
	uint32 *dP;
	uint16 *bP;
	uint16 *xP;

#ifdef MMX
//  if (mmx_cpu && width != 512)
	for (height; height; height-=1)
	{

		//sprintfx( "2xsai %u\r\n", height ) ;
	    bP = (uint16 *) srcPtr;
	    xP = (uint16 *) deltaPtr;
	    dP = (uint32 *) dstPtr;

		_2xSaILineW((uint8 *) bP, (uint8 *) xP, srcPitch, width, (uint8 *)dP, dstPitch);

		if ( height & 0x01 )
		{
			memset( dstPtr, 0, width<<2 ) ;
			//memset( dstPtr+dstPitch, 0, width<<2 ) ;
			//memset( bP, 0, width<<1 ) ;
			//memset( bP+srcPitch, 0, width<<1 ) ;
		}
		//else
		//{
			//_2xSaILineW((uint8 *) bP, (uint8 *) xP, srcPitch, width, (uint8 *)dP, dstPitch);
		//}
	    dstPtr += dstPitch << 1;
	    srcPtr += srcPitch;
			deltaPtr += srcPitch;
	}
#else
		uint32 Nextline = srcPitch >> 1;

		Init_2xSaI(565);

		for (height; height; height-=1)
		{
	   	    uint32 finish;

	        bP = (uint16 *) srcPtr;
	        dP = (uint32 *) dstPtr;
			for (finish = width; finish; finish -= 1 )
			{
				register uint32 colorA, colorB;
				uint32 colorC, colorD,
					   colorE, colorF, colorG, colorH,
					   colorI, colorJ, colorK, colorL,
					   colorM, colorN, colorO, colorP;
				uint32 product, product1, product2;


//---------------------------------------
// Map of the pixels:                    I|E F|J
//                                       G|A B|K
//                                       H|C D|L
//                                       M|N O|P
				colorI = *(bP- Nextline - 1);
				colorE = *(bP- Nextline);
				colorF = *(bP- Nextline + 1);
				colorJ = *(bP- Nextline + 2);

				colorG = *(bP - 1);
				colorA = *(bP);
				colorB = *(bP + 1);
				colorK = *(bP + 2);

				colorH = *(bP + Nextline - 1);
				colorC = *(bP + Nextline);
				colorD = *(bP + Nextline + 1);
				colorL = *(bP + Nextline + 2);

				colorM = *(bP + Nextline + Nextline - 1);
				colorN = *(bP + Nextline + Nextline);
				colorO = *(bP + Nextline + Nextline + 1);
				colorP = *(bP + Nextline + Nextline + 2);

						if ((colorA == colorD) && (colorB != colorC))
						{
						   if ( ((colorA == colorE) && (colorB == colorL)) ||
								((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ)) )
						   {
							  product = colorA;
						   }
						   else
						   {
							  product = INTERPOLATE(colorA, colorB);
						   }

						   if (((colorA == colorG) && (colorC == colorO)) ||
							   ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM)) )
						   {
							  product1 = colorA;
						   }
						   else
						   {
							  product1 = INTERPOLATE(colorA, colorC);
						   }
						   product2 = colorA;
						}
						else
						if ((colorB == colorC) && (colorA != colorD))
						{
						   if (((colorB == colorF) && (colorA == colorH)) ||
							   ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI)) )
						   {
							  product = colorB;
						   }
						   else
						   {
							  product = INTERPOLATE(colorA, colorB);
						   }

						   if (((colorC == colorH) && (colorA == colorF)) ||
							   ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI)) )
						   {
							  product1 = colorC;
						   }
						   else
						   {
							  product1 = INTERPOLATE(colorA, colorC);
						   }
						   product2 = colorB;
						}
						else
						if ((colorA == colorD) && (colorB == colorC))
						{
						   if (colorA == colorB)
						   {
							  product = colorA;
							  product1 = colorA;
							  product2 = colorA;
						   }
						   else
						   {
							  register int r = 0;
							  product1 = INTERPOLATE(colorA, colorC);
							  product = INTERPOLATE(colorA, colorB);

							  r += GetResult1 (colorA, colorB, colorG, colorE, colorI);
							  r += GetResult2 (colorB, colorA, colorK, colorF, colorJ);
							  r += GetResult2 (colorB, colorA, colorH, colorN, colorM);
							  r += GetResult1 (colorA, colorB, colorL, colorO, colorP);

							  if (r > 0)
								  product2 = colorA;
							  else
							  if (r < 0)
								  product2 = colorB;
							  else
							  {
								  product2 = Q_INTERPOLATE(colorA, colorB, colorC, colorD);
							  }
						   }
						}
						else
						{
						   product2 = Q_INTERPOLATE(colorA, colorB, colorC, colorD);

						   if ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ))
						   {
							  product = colorA;
						   }
						   else
						   if ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI))
						   {
							  product = colorB;
						   }
						   else
						   {
							  product = INTERPOLATE(colorA, colorB);
						   }

						   if ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM))
						   {
							  product1 = colorA;
						   }
						   else
						   if ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI))
						   {
							  product1 = colorC;
						   }
						   else
						   {
							  product1 = INTERPOLATE(colorA, colorC);
						   }
						}
						product = colorA | (product << 16);
						product1 = product1 | (product2 << 16);

			*(dP) = product;
			*(dP+(dstPitch>>2)) = product1;

					bP += 1;
					dP += 1;
				}//end of for ( finish= width etc..)

			dstPtr += dstPitch << 1;
			srcPtr += srcPitch;
			deltaPtr += srcPitch;
	}; //endof: for (height; height; height--)
#endif //MMX
}

extern void Scale2x(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode)
{
	uint32 *dP;
	uint16 *bP;
	uint32 Nextline = srcPitch >> 1;

	for (height; height; height-=1)
	{
	    uint32 finish;

	    bP = (uint16 *) srcPtr;
	    dP = (uint32 *) dstPtr;

		for (finish = width; finish; finish -= 1 )
		{

			register uint32 colorA, colorB;
			uint32 colorC, colorD,
				   colorE, colorF, colorG, colorH,
				   colorI;
			uint32 product1, product2, product1b, product2b;


//---------------------------------------
// Map of the pixels:             A|B C|J
//                                D|E F|K
//                                G|H D|L
//                                M|N O|P

			colorA = *(bP- Nextline - 1);
			colorB = *(bP- Nextline);
			colorC = *(bP- Nextline + 1);

			colorD = *(bP - 1);
			colorE = *(bP);
			colorF = *(bP + 1);

			colorG = *(bP + Nextline - 1);
			colorH = *(bP + Nextline);
			colorI = *(bP + Nextline + 1);

			switch (scanmode)
			{
			case 2 :
				/* version 2 */
				product1 = colorD == colorB && ((colorB != colorF && colorD != colorH) || colorD == colorA) ? colorD : colorE;
				product1b = colorB == colorF && ((colorB != colorD && colorF != colorH) || colorB == colorC) ? colorF : colorE;
				product2 = colorD == colorH && ((colorD != colorB && colorH != colorF) || colorD == colorG) ? colorD : colorE;
				product2b = colorH == colorF && ((colorD != colorH && colorB != colorF) || colorH == colorI) ? colorF : colorE;
				break;
			case 3 :
				/* version 3 */
				product1 = colorD == colorB && colorA != colorE ? colorD : colorE;
				product1b = colorB == colorF && colorC != colorE ? colorF : colorE;
				product2 = colorD == colorH && colorG != colorE ? colorD : colorE;
				product2b = colorH == colorF && colorI != colorE ? colorF : colorE;
				break;
			case 1 :
			default :
				/* version 1 */
				product1 = colorD == colorB && colorB != colorF && colorD != colorH ? colorD : colorE;
				product1b = colorB == colorF && colorB != colorD && colorF != colorH ? colorF : colorE;
				product2 = colorD == colorH && colorD != colorB && colorH != colorF ? colorD : colorE;
				product2b = colorH == colorF && colorD != colorH && colorB != colorF ? colorF : colorE;
				break;
			}

			product1 = product1 | (product1b << 16);
			product2 = product2 | (product2b << 16);


			*(dP) = product1;
			*(dP+(dstPitch>>2)) = product2;

			bP += 1;
			dP += 1;
		}//end of for ( finish= width etc..)

		dstPtr += dstPitch << 1;
		srcPtr += srcPitch;
		deltaPtr += srcPitch;
	}; //endof: for (height; height; height--)
}

extern void SuperScaleScanline(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode)
{
	uint32 *dP;
	uint16 *bP;
//  uint16 *xP;
	unsigned __int64 mask;

//	if (Is555)
	if (0)
		mask=0x3DEF3DEF3DEF3DEF;
	else
		mask=0x7BEF7BEF7BEF7BEF;

#ifdef MMX
	if (true)//mmx_cpu && width != 512)
	{
   	    uint32 srcNextline = srcPitch >> 1;
	    uint16 *dst0=(uint16 *)dstPtr;
	    uint16 *dst1=(uint16 *)(dstPtr + dstPitch);
	    uint16 *src0=(uint16 *)(srcPtr -srcPitch);  //don't worry, there is extra space :)
	    uint16 *src1=(uint16 *)srcPtr;
	    uint16 *src2=(uint16 *)(srcPtr + srcPitch);
	    int i;
	    for(i=0;i<height;++i)
	    {
			//sprintfx( "superscale %u %u\r\n", height, width) ;
	   	 _2xSaISuperScaleLineW(src0,src1,src2,dst0,width);
	   	 //_2xSaISuperScaleLineW(src0,src1,src2,dst0,width);
			//sprintfx( "superscale %u\r\n", height) ;
		//if (scanmode != 0 )
	        _2xSaISuperScale75LineW(src2,src1,src0,dst1,width,&mask);
		//else
			//_2xSaISuperScaleLineW(src2,src1,src0,dst1,width);
			//sprintfx( "superscale %u\r\n", height) ;
	   	 src0 = src1;
	        src1 = src2;
	   	 src2 += srcNextline;
	        dst0 += dstPitch;
	   	 dst1 += dstPitch;
	    }
#ifdef _MSC_VER
	    __asm {
	        emms
	    };
#else
	    asm("emms");
#endif
	}
	else
	{
#endif

		uint32 Nextline = srcPitch >> 1;
		uint32 dstNextline = dstPitch >> 2;

		for (height; height; height-=1)
		{
		    uint32 finish;

			bP = (uint16 *) srcPtr;
	     	dP = (uint32 *) dstPtr;

			for (finish = width; finish; finish -= 1 )
			{
				register uint32 colorH, colorB;
				uint32 colorA, colorC, colorD,
					   colorE, colorF, colorG,
					   colorI;
				uint32 product1, product2, product1b, product2b;


//---------------------------------------
// Map of the pixels:             A|B C|J
//                                D|E F|K
//                                G|H I|L
//                                M|N O|P
//;E0=(B==D && B!=F && B!=H)?B:E;
//;E1=(B!=D && B==F && B!=H)?B:E;

				colorA = *(bP- Nextline - 1);
				colorB = *(bP- Nextline);
				colorC = *(bP- Nextline + 1);

				colorD = *(bP - 1);
				colorE = *(bP);
				colorF = *(bP + 1);

				colorG = *(bP + Nextline - 1);
				colorH = *(bP + Nextline);
				colorI = *(bP + Nextline + 1);

				product1  = colorB == colorD && colorB != colorF && colorB != colorH ? colorB : colorE;
				product1b = colorB != colorD && colorB == colorF && colorB != colorH ? colorB : colorE;
				product2 = colorH == colorD && colorH != colorF && colorB != colorH ? colorH : colorE;
				product2b = colorH != colorD && colorH == colorF && colorB != colorH ? colorH : colorE;

				product1 = product1 | (product1b << 16);
				product2 = product2 | (product2b << 16);

				*(dP) = product1;
				*(dP + dstNextline) = product2;

				bP += 1;
				dP += 1;
			}//end of for ( finish= width etc..)

			dstPtr += dstPitch << 1;
			srcPtr += srcPitch;
//          deltaPtr += srcPitch;
	}; //endof: for (height; height; height--)
#ifdef MMX
	}
#endif
}

extern void SuperScale(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode)
{
	uint32 *dP;
	uint16 *bP;
//  uint16 *xP;
	unsigned __int64 mask;

//	if (Is555)
	if (0)
		mask=0x3DEF3DEF3DEF3DEF;
	else
		mask=0x7BEF7BEF7BEF7BEF;

#ifdef MMX
	if (true)//mmx_cpu && width != 512)
	{
   	    uint32 srcNextline = srcPitch >> 1;
	    uint16 *dst0=(uint16 *)dstPtr;
	    uint16 *dst1=(uint16 *)(dstPtr + dstPitch);
	    uint16 *src0=(uint16 *)(srcPtr -srcPitch);  //don't worry, there is extra space :)
	    uint16 *src1=(uint16 *)srcPtr;
	    uint16 *src2=(uint16 *)(srcPtr + srcPitch);
	    int i;
	    for(i=0;i<height;++i)
	    {
			//sprintfx( "superscale %u %u\r\n", height, width) ;
	   	 _2xSaISuperScaleLineW(src0,src1,src2,dst0,width);
	   	 //_2xSaISuperScaleLineW(src0,src1,src2,dst0,width);
			//sprintfx( "superscale %u\r\n", height) ;
		//if (scanmode != 0 )
	        //_2xSaISuperScale75LineW(src2,src1,src0,dst1,width,&mask);
		//else
			_2xSaISuperScaleLineW(src2,src1,src0,dst1,width);
			//sprintfx( "superscale %u\r\n", height) ;
	   	 src0 = src1;
	        src1 = src2;
	   	 src2 += srcNextline;
	        dst0 += dstPitch;
	   	 dst1 += dstPitch;
	    }
#ifdef _MSC_VER
	    __asm {
	        emms
	    };
#else
	    asm("emms");
#endif
	}
	else
	{
#endif

		uint32 Nextline = srcPitch >> 1;
		uint32 dstNextline = dstPitch >> 2;

		for (height; height; height-=1)
		{
		    uint32 finish;

			bP = (uint16 *) srcPtr;
	     	dP = (uint32 *) dstPtr;

			for (finish = width; finish; finish -= 1 )
			{
				register uint32 colorH, colorB;
				uint32 colorA, colorC, colorD,
					   colorE, colorF, colorG,
					   colorI;
				uint32 product1, product2, product1b, product2b;


//---------------------------------------
// Map of the pixels:             A|B C|J
//                                D|E F|K
//                                G|H I|L
//                                M|N O|P
//;E0=(B==D && B!=F && B!=H)?B:E;
//;E1=(B!=D && B==F && B!=H)?B:E;

				colorA = *(bP- Nextline - 1);
				colorB = *(bP- Nextline);
				colorC = *(bP- Nextline + 1);

				colorD = *(bP - 1);
				colorE = *(bP);
				colorF = *(bP + 1);

				colorG = *(bP + Nextline - 1);
				colorH = *(bP + Nextline);
				colorI = *(bP + Nextline + 1);

				product1  = colorB == colorD && colorB != colorF && colorB != colorH ? colorB : colorE;
				product1b = colorB != colorD && colorB == colorF && colorB != colorH ? colorB : colorE;
				product2 = colorH == colorD && colorH != colorF && colorB != colorH ? colorH : colorE;
				product2b = colorH != colorD && colorH == colorF && colorB != colorH ? colorH : colorE;

				product1 = product1 | (product1b << 16);
				product2 = product2 | (product2b << 16);

				*(dP) = product1;
				*(dP + dstNextline) = product2;

				bP += 1;
				dP += 1;
			}//end of for ( finish= width etc..)

			dstPtr += dstPitch << 1;
			srcPtr += srcPitch;
//          deltaPtr += srcPitch;
	}; //endof: for (height; height; height--)
#ifdef MMX
	}
#endif
}


extern void Eagle(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode)
{
#ifdef MMX
	if (true)//mmx_cpu && width != 512)
	{
	uint8 *src0=srcPtr;
	uint8 *src1=srcPtr + srcPitch;
	uint16 *dst0=(uint16 *)dstPtr;
	uint16 *dst1=(uint16 *)(dstPtr + dstPitch);
	uint32 w=width<<1;
	int i;
	for(i=0;i<height;i++)
	{
		_2xSaIEagleLineW(src0, src1, w, dst0, dst1);
		src0 = src1;
		src1 += srcPitch;
		dst0 += dstPitch;
		dst1 += dstPitch;
	}
#ifdef _MSC_VER
	__asm {
	    emms
	};
#else
	asm("emms");
#endif
	}
#endif
}


extern void EagleScanline(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode)
{
#ifdef MMX
	if (true)//mmx_cpu && width != 512)
	{
	uint8 *src0=srcPtr;
	uint8 *src1=srcPtr + srcPitch;
	uint16 *dst0=(uint16 *)dstPtr;
	uint16 *dst1=(uint16 *)(dstPtr + dstPitch);
	uint32 w=width<<1;
	int i;
	for(i=0;i<height;i++)
	{
		_2xSaIEagleLineW(src0, src1, w, dst0, dst1);
		if ( i & 0x01 )
		{
			memset( dst0, 0, width<<2 ) ;
			//memset( dstPtr+dstPitch, 0, width<<2 ) ;
			//memset( bP, 0, width<<1 ) ;
			//memset( bP+srcPitch, 0, width<<1 ) ;
		}
		src0 = src1;
		src1 += srcPitch;
		dst0 += dstPitch;
		dst1 += dstPitch;
	}
#ifdef _MSC_VER
	__asm {
	    emms
	};
#else
	asm("emms");
#endif
	}
#endif
}


