/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#include "gfx.h"
#include "borendian.h"
#include "gfxtypes.h"

#ifdef MMX
extern void __Init_2xSaIMMX (u32 BitFormat);
extern void __2xSaILine (u8 *srcPtr, u8 *deltaPtr, u32 srcPitch, u32 width, u8 *dstPtr, u32 dstPitch);
extern void __2xSaISuperEagleLine (u8 *srcPtr, u8 *deltaPtr, u32 srcPitch, u32 width, u8 *dstPtr, u32 dstPitch);
extern void __2xSaISuper2xSaILine (u8 *srcPtr, u8 *deltaPtr, u32 srcPitch, u32 width, u8 *dstPtr, u32 dstPitch);
#endif

u32 GfxColorMask = 0xF7DEF7DE;
u32 GfxLowPixelMask = 0x08210821;
u32 GfxQColorMask = 0xE79CE79C;
u32 GfxQLowPixelMask = 0x18631863;
u32 RGB_LOW_BITS_MASK = 0x0821;
u32 GfxRedBlueMask = 0xF81F;
u32 GfxGreenMask = 0x7E0;
u32 GfxColorDepth = 16;
u32 GfxRedShift = 11;
u32 GfxGreenShift = 6;
u32 GfxBlueShift = 0;
u32 qRGB_COLOR_MASK[2] = { 0xF7DEF7DE, 0xF7DEF7DE };

extern void Init_Hq2x (u32, u32);
extern void Term_Hq2x ();

/* Queries the CPU using cpuid to determine whether it supports MMX.  Returns 1
 * if the processor supports MMX, 0 if it doesn't.  Will always return 0 if the
 * CPU architecture is anything other than x86. */
bool GetMMX ()
{
#ifdef MMX
    int retval;
    __asm__ (
        "push %%ebx\n"
        "mov $1, %%eax\n"
        "cpuid\n"
        "shr $23, %%edx\n"
        "and $1, %%edx\n"
        "pop %%ebx\n"
        : "=d"(retval)
        : // no inputs
        : "cc", "eax", "ecx"
    );
    return retval;
#else
    return 0;
#endif
}

int Init_2xSaI (u32 BitFormat, u32 ColorDepth)
{
    if (ColorDepth == 16)
    {
        if (BitFormat == 565)
        {
            GfxColorMask = 0xF7DEF7DE;
            GfxLowPixelMask = 0x08210821;
            GfxQColorMask = 0xE79CE79C;
            GfxQLowPixelMask = 0x18631863;
            GfxRedBlueMask = 0xF81F;
            GfxGreenMask = 0x7E0;
            qRGB_COLOR_MASK[0] = qRGB_COLOR_MASK[1] = 0xF7DEF7DE;
            GfxRedShift = 11;
            GfxGreenShift = 6;
            GfxBlueShift = 0;
            GfxColorDepth = ColorDepth;
            Init_Hq2x(BitFormat, GfxColorDepth);
            RGB_LOW_BITS_MASK = 0x0821;
        }
        else if (BitFormat == 555)
        {
            GfxColorMask  = 0x7BDE7BDE;
            GfxLowPixelMask = 0x04210421;
            GfxQColorMask = 0x739C739C;
            GfxQLowPixelMask = 0x0C630C63;
            GfxRedBlueMask = 0x7C1F;
            GfxGreenMask = 0x3E0;
            qRGB_COLOR_MASK[0] = qRGB_COLOR_MASK[1] = 0x7BDE7BDE;
            GfxRedShift = 10;
            GfxGreenShift = 5;
            GfxBlueShift = 0;
            GfxColorDepth = ColorDepth - 1;
            Init_Hq2x(BitFormat, GfxColorDepth);
            RGB_LOW_BITS_MASK = 0x0421;
        }
        else
        {
            return 0;
        }
    }
    else if (ColorDepth == 32)
    {
        GfxColorMask  = 0xfefefe;
        GfxLowPixelMask = 0x010101;
        GfxQColorMask = 0xfcfcfc;
        GfxQLowPixelMask = 0x030303;
        qRGB_COLOR_MASK[0] = qRGB_COLOR_MASK[1] = 0xfefefe;
        GfxRedShift = 19;
        GfxGreenShift = 11;
        GfxBlueShift = 3;
        GfxColorDepth = ColorDepth;
        Init_Hq2x(BitFormat, GfxColorDepth);
        RGB_LOW_BITS_MASK = 0x010101;
    }
    else
    {
        return 0;
    }

#ifdef MMX
    __Init_2xSaIMMX (BitFormat);
#endif

    return 1;
}

int Init_Gfx (u32 BitFormat, u32 ColorDepth)
{
    return Init_2xSaI(BitFormat, ColorDepth);
}

void Term_Gfx ()
{
    Term_Hq2x();

#ifdef MMX
    // terminate MMX and restore floating point processing
    __asm__ ("emms");
#endif
}

static inline int GetResult1 (u32 A, u32 B, u32 C, u32 D, u32 E)
{
    int x = 0;
    int y = 0;
    int r = 0;

    if (A == C)
    {
        x += 1;
    }
    else if (B == C)
    {
        y += 1;
    }
    if (A == D)
    {
        x += 1;
    }
    else if (B == D)
    {
        y += 1;
    }
    if (x <= 1)
    {
        r += 1;
    }
    if (y <= 1)
    {
        r -= 1;
    }
    return r;
}

static inline int GetResult2 (u32 A, u32 B, u32 C, u32 D, u32 E)
{
    int x = 0;
    int y = 0;
    int r = 0;

    if (A == C)
    {
        x += 1;
    }
    else if (B == C)
    {
        y += 1;
    }
    if (A == D)
    {
        x += 1;
    }
    else if (B == D)
    {
        y += 1;
    }
    if (x <= 1)
    {
        r -= 1;
    }
    if (y <= 1)
    {
        r += 1;
    }
    return r;
}

static inline int GetResult (u32 A, u32 B, u32 C, u32 D)
{
    int x = 0;
    int y = 0;
    int r = 0;

    if (A == C)
    {
        x += 1;
    }
    else if (B == C)
    {
        y += 1;
    }
    if (A == D)
    {
        x += 1;
    }
    else if (B == D)
    {
        y += 1;
    }
    if (x <= 1)
    {
        r += 1;
    }
    if (y <= 1)
    {
        r -= 1;
    }
    return r;
}

static inline u32 INTERPOLATE (u32 A, u32 B)
{
    if (A != B)
    {
        return (((A & GfxColorMask) >> 1) + ((B & GfxColorMask) >> 1) + (A & B & GfxLowPixelMask));
    }
    else
    {
        return A;
    }
}

static inline u32 Q__INTERPOLATE (u32 A, u32 B, u32 C, u32 D)
{
    register u32 x = ((A & GfxQColorMask) >> 2) + ((B & GfxQColorMask) >> 2) + ((C & GfxQColorMask) >> 2) + ((D & GfxQColorMask) >> 2);
    register u32 y = (A & GfxQLowPixelMask) + (B & GfxQLowPixelMask) + (C & GfxQLowPixelMask) + (D & GfxQLowPixelMask);
    y = (y >> 2) & GfxQLowPixelMask;
    return x + y;
}

static inline int GetResult1_32 (u32 A, u32 B, u32 C, u32 D, u32 E)
{
    int x = 0;
    int y = 0;
    int r = 0;

    if (A == C)
    {
        x += 1;
    }
    else if (B == C)
    {
        y += 1;
    }
    if (A == D)
    {
        x += 1;
    }
    else if (B == D)
    {
        y += 1;
    }
    if (x <= 1)
    {
        r += 1;
    }
    if (y <= 1)
    {
        r -= 1;
    }
    return r;
}

static inline int GetResult2_32 (u32 A, u32 B, u32 C, u32 D, u32 E)
{
    int x = 0;
    int y = 0;
    int r = 0;

    if (A == C)
    {
        x += 1;
    }
    else if (B == C)
    {
        y += 1;
    }
    if (A == D)
    {
        x += 1;
    }
    else if (B == D)
    {
        y += 1;
    }
    if (x <= 1)
    {
        r -= 1;
    }
    if (y <= 1)
    {
        r += 1;
    }
    return r;
}

#define BLUE_MASK565  0x001F001F
#define RED_MASK565   0xF800F800
#define GREEN_MASK565 0x07E007E0

#define BLUE_MASK555  0x001F001F
#define RED_MASK555   0x7C007C00
#define GREEN_MASK555 0x03E003E0

void Super2xSaI (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
    u16 *bP;
    u8  *dP;
    u32 finish;
    u32 inc__BP = 1;
    u32 Nextline = srcPitch >> 1;

#ifdef MMX
    if (GetMMX())
    {
        for (; height; height--)
        {
            __2xSaISuper2xSaILine (srcPtr, deltaPtr, srcPitch, width, dstPtr, dstPitch);
            srcPtr += srcPitch;
            dstPtr += dstPitch * 2;
            deltaPtr += srcPitch;
        }
    }
    else
#endif
    {
        for (; height; height--)
        {
            bP = (u16 *) srcPtr;
            dP = (u8 *) dstPtr;

            for (finish = width; finish; finish -= inc__BP)
            {
                u32 color4, color5, color6;
                u32 color1, color2, color3;
                u32 colorA0, colorA1, colorA2, colorA3,
                    colorB0, colorB1, colorB2, colorB3, colorS1, colorS2;
                u32 product1a, product1b, product2a, product2b;

//------------------------    B1 B2
//                          4  5  6 S2
//                          1  2  3 S1
//                            A1 A2

                colorB0 = *(bP - Nextline - 1);
                colorB1 = *(bP - Nextline);
                colorB2 = *(bP - Nextline + 1);
                colorB3 = *(bP - Nextline + 2);

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
                else if (color5 == color3 && color2 != color6)
                {
                    product2b = product1b = color5;
                }
                else if (color5 == color3 && color2 == color6)
                {
                    register int r = 0;
                    r += GetResult (color6, color5, color1, colorA1);
                    r += GetResult (color6, color5, color4, colorB1);
                    r += GetResult (color6, color5, colorA2, colorS1);
                    r += GetResult (color6, color5, colorB2, colorS2);

                    if (r > 0)
                    {
                        product2b = product1b = color6;
                    }
                    else if (r < 0)
                    {
                        product2b = product1b = color5;
                    }
                    else
                    {
                        product2b = product1b = INTERPOLATE (color5, color6);
                    }
                }
                else
                {
                    if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
                    {
                        product2b = Q__INTERPOLATE (color3, color3, color3, color2);
                    }
                    else if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
                    {
                        product2b = Q__INTERPOLATE (color2, color2, color2, color3);
                    }
                    else
                    {
                        product2b = INTERPOLATE (color2, color3);
                    }

                    if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
                    {
                        product1b = Q__INTERPOLATE (color6, color6, color6, color5);
                    }
                    else if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
                    {
                        product1b = Q__INTERPOLATE (color6, color5, color5, color5);
                    }
                    else
                    {
                        product1b = INTERPOLATE (color5, color6);
                    }
                }

                if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
                {
                    product2a = INTERPOLATE (color2, color5);
                }
                else if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
                {
                    product2a = INTERPOLATE (color2, color5);
                }
                else
                {
                    product2a = color2;
                }

                if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
                {
                    product1a = INTERPOLATE (color2, color5);
                }
                else if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
                {
                    product1a = INTERPOLATE (color2, color5);
                }
                else
                {
                    product1a = color5;
                }

#ifdef BIG__ENDIAN
                product1a = (product1a << 16) | product1b;
                product2a = (product2a << 16) | product2b;
#else
                product1a = product1a | (product1b << 16);
                product2a = product2a | (product2b << 16);
#endif

                *((u32 *) dP) = product1a;
                *((u32 *) (dP + dstPitch)) = product2a;

                bP += inc__BP;
                dP += sizeof (u32);
            }
            // end of for ( finish= width etc..)

            srcPtr   += srcPitch;
            dstPtr   += dstPitch << 1;
            deltaPtr += srcPitch;

        }
        // endof: for (; height; height--)
    }
}

void Super2xSaI32 (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
    u32 *bP;
    u32 *dP;
    u32 finish;
    u32 inc__BP = 1;
    u32 Nextline = srcPitch >> 2;


    for (; height; height--)
    {
        bP = (u32 *) srcPtr;
        dP = (u32 *) dstPtr;

        for (finish = width; finish; finish -= inc__BP)
        {
            u32 color4, color5, color6;
            u32 color1, color2, color3;
            u32 colorA0, colorA1, colorA2, colorA3,
                colorB0, colorB1, colorB2, colorB3, colorS1, colorS2;
            u32 product1a, product1b, product2a, product2b;

//-------------------------    B1 B2
//                          4  5  6 S2
//                          1  2  3 S1
//                            A1 A2

            colorB0 = *(bP - Nextline - 1);
            colorB1 = *(bP - Nextline);
            colorB2 = *(bP - Nextline + 1);
            colorB3 = *(bP - Nextline + 2);

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
            else if (color5 == color3 && color2 != color6)
            {
                product2b = product1b = color5;
            }
            else if (color5 == color3 && color2 == color6)
            {
                register int r = 0;
                r += GetResult (color6, color5, color1, colorA1);
                r += GetResult (color6, color5, color4, colorB1);
                r += GetResult (color6, color5, colorA2, colorS1);
                r += GetResult (color6, color5, colorB2, colorS2);

                if (r > 0)
                {
                    product2b = product1b = color6;
                }
                else if (r < 0)
                {
                    product2b = product1b = color5;
                }
                else
                {
                    product2b = product1b = INTERPOLATE (color5, color6);
                }
            }
            else
            {
                if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
                {
                    product2b = Q__INTERPOLATE (color3, color3, color3, color2);
                }
                else if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
                {
                    product2b = Q__INTERPOLATE (color2, color2, color2, color3);
                }
                else
                {
                    product2b = INTERPOLATE (color2, color3);
                }

                if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
                {
                    product1b = Q__INTERPOLATE (color6, color6, color6, color5);
                }
                else if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
                {
                    product1b = Q__INTERPOLATE (color6, color5, color5, color5);
                }
                else
                {
                    product1b = INTERPOLATE (color5, color6);
                }
            }

            if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
            {
                product2a = INTERPOLATE (color2, color5);
            }
            else if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
            {
                product2a = INTERPOLATE (color2, color5);
            }
            else
            {
                product2a = color2;
            }

            if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
            {
                product1a = INTERPOLATE (color2, color5);
            }
            else if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
            {
                product1a = INTERPOLATE (color2, color5);
            }
            else
            {
                product1a = color5;
            }

            *(dP) = product1a;
            *(dP + 1) = product1b;
            *(dP + (dstPitch >> 2)) = product2a;
            *(dP + (dstPitch >> 2) + 1) = product2b;

            bP += inc__BP;
            dP += 2;
        }
        // end of for ( finish= width etc..)

        srcPtr   += srcPitch;
        dstPtr   += dstPitch << 1;
        //deltaPtr += srcPitch;
    }
    // endof: for (; height; height--)
}

void SuperEagle (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
    u8  *dP;
    u16 *bP;
    u16 *xP;
    u32 finish;
    u32 inc__BP = 1;
    u32 Nextline = srcPitch >> 1;

#ifdef MMX
    if (GetMMX())
    {
        for (; height; height--)
        {
            __2xSaISuperEagleLine (srcPtr, deltaPtr, srcPitch, width, dstPtr, dstPitch);
            srcPtr += srcPitch;
            dstPtr += dstPitch * 2;
            deltaPtr += srcPitch;
        }
    }
    else
#endif
    {
        for (; height; height--)
        {
            bP = (u16 *) srcPtr;
            xP = (u16 *) deltaPtr;
            dP = dstPtr;
            for (finish = width; finish; finish -= inc__BP)
            {
                u32 color4, color5, color6;
                u32 color1, color2, color3;
                u32 colorA1, colorA2, colorB1, colorB2, colorS1, colorS2;
                u32 product1a, product1b, product2a, product2b;

                colorB1 = *(bP - Nextline);
                colorB2 = *(bP - Nextline + 1);

                color4 = *(bP - 1);
                color5 = *(bP);
                color6 = *(bP + 1);
                colorS2 = *(bP + 2);

                color1 = *(bP + Nextline - 1);
                color2 = *(bP + Nextline);
                color3 = *(bP + Nextline + 1);
                colorS1 = *(bP + Nextline + 2);

                colorA1 = *(bP + Nextline + Nextline);
                colorA2 = *(bP + Nextline + Nextline + 1);

// --------------------------------------
                if (color2 == color6 && color5 != color3)
                {
                    product1b = product2a = color2;
                    if ((color1 == color2) || (color6 == colorB2))
                    {
                        product1a = INTERPOLATE (color2, color5);
                        product1a = INTERPOLATE (color2, product1a);
                    }
                    else
                    {
                        product1a = INTERPOLATE (color5, color6);
                    }

                    if ((color6 == colorS2) || (color2 == colorA1))
                    {
                        product2b = INTERPOLATE (color2, color3);
                        product2b = INTERPOLATE (color2, product2b);
                    }
                    else
                    {
                        product2b = INTERPOLATE (color2, color3);
                    }
                }
                else if (color5 == color3 && color2 != color6)
                {
                    product2b = product1a = color5;
                    if ((colorB1 == color5) || (color3 == colorS1))
                    {
                        product1b = INTERPOLATE (color5, color6);
                        product1b = INTERPOLATE (color5, product1b);
                    }
                    else
                    {
                        product1b = INTERPOLATE (color5, color6);
                    }

                    if ((color3 == colorA2) || (color4 == color5))
                    {
                        product2a = INTERPOLATE (color5, color2);
                        product2a = INTERPOLATE (color5, product2a);
                    }
                    else
                    {
                        product2a = INTERPOLATE (color2, color3);
                    }
                }
                else if (color5 == color3 && color2 == color6)
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
                    else if (r < 0)
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
                    product2b = product1a = INTERPOLATE (color2, color6);
                    product2b = Q__INTERPOLATE (color3, color3, color3, product2b);
                    product1a = Q__INTERPOLATE (color5, color5, color5, product1a);
                    product2a = product1b = INTERPOLATE (color5, color3);
                    product2a = Q__INTERPOLATE (color2, color2, color2, product2a);
                    product1b = Q__INTERPOLATE (color6, color6, color6, product1b);
                }

#ifdef BIG__ENDIAN
                product1a = (product1a << 16) | product1b;
                product2a = (product2a << 16) | product2b;
#else
                product1a = product1a | (product1b << 16);
                product2a = product2a | (product2b << 16);
#endif

                *((u32 *) dP) = product1a;
                *((u32 *) (dP + dstPitch)) = product2a;
                *xP = color5;

                bP += inc__BP;
                xP += inc__BP;
                dP += sizeof (u32);
            }
            // end of for ( finish= width etc..)

            srcPtr += srcPitch;
            dstPtr += dstPitch << 1;
            deltaPtr += srcPitch;
        }
        // endof: for (height; height; height--)
    }
}

void SuperEagle32 (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
    u32 *dP;
    u32 *bP;
    u32 *xP;
    u32 finish;
    u32 inc__BP = 1;
    u32 Nextline = srcPitch >> 2;

    for (; height; height--)
    {
        bP = (u32 *) srcPtr;
        xP = (u32 *) deltaPtr;
        dP = (u32 *) dstPtr;
        for (finish = width; finish; finish -= inc__BP)
        {
            u32 color4, color5, color6;
            u32 color1, color2, color3;
            u32 colorA1, colorA2, colorB1, colorB2, colorS1, colorS2;
            u32 product1a, product1b, product2a, product2b;

            colorB1 = *(bP - Nextline);
            colorB2 = *(bP - Nextline + 1);

            color4 = *(bP - 1);
            color5 = *(bP);
            color6 = *(bP + 1);
            colorS2 = *(bP + 2);

            color1 = *(bP + Nextline - 1);
            color2 = *(bP + Nextline);
            color3 = *(bP + Nextline + 1);
            colorS1 = *(bP + Nextline + 2);

            colorA1 = *(bP + Nextline + Nextline);
            colorA2 = *(bP + Nextline + Nextline + 1);

// --------------------------------------

            if (color2 == color6 && color5 != color3)
            {
                product1b = product2a = color2;
                if ((color1 == color2) || (color6 == colorB2))
                {
                    product1a = INTERPOLATE (color2, color5);
                    product1a = INTERPOLATE (color2, product1a);
                }
                else
                {
                    product1a = INTERPOLATE (color5, color6);
                }

                if ((color6 == colorS2) || (color2 == colorA1))
                {
                    product2b = INTERPOLATE (color2, color3);
                    product2b = INTERPOLATE (color2, product2b);
                }
                else
                {
                    product2b = INTERPOLATE (color2, color3);
                }
            }
            else if (color5 == color3 && color2 != color6)
            {
                product2b = product1a = color5;

                if ((colorB1 == color5) || (color3 == colorS1))
                {
                    product1b = INTERPOLATE (color5, color6);
                    product1b = INTERPOLATE (color5, product1b);
                }
                else
                {
                    product1b = INTERPOLATE (color5, color6);
                }

                if ((color3 == colorA2) || (color4 == color5))
                {
                    product2a = INTERPOLATE (color5, color2);
                    product2a = INTERPOLATE (color5, product2a);
                }
                else
                {
                    product2a = INTERPOLATE (color2, color3);
                }
            }
            else if (color5 == color3 && color2 == color6)
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
                else if (r < 0)
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
                product2b = product1a = INTERPOLATE (color2, color6);
                product2b = Q__INTERPOLATE (color3, color3, color3, product2b);
                product1a = Q__INTERPOLATE (color5, color5, color5, product1a);
                product2a = product1b = INTERPOLATE (color5, color3);
                product2a = Q__INTERPOLATE (color2, color2, color2, product2a);
                product1b = Q__INTERPOLATE (color6, color6, color6, product1b);
            }

            *(dP) = product1a;
            *(dP + 1) = product1b;
            *(dP + (dstPitch >> 2)) = product2a;
            *(dP + (dstPitch >> 2) + 1) = product2b;
            *xP = color5;

            bP += inc__BP;
            xP += inc__BP;
            dP += 2;
        }
        // end of for ( finish= width etc..)

        srcPtr += srcPitch;
        dstPtr += dstPitch << 1;
        deltaPtr += srcPitch;
    }
    // endof: for (height; height; height--)
}

void _2xSaI (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
    u8  *dP;
    u16 *bP;
    u32 finish;
    u32 inc__BP = 1;
    u32 Nextline = srcPitch >> 1;

#ifdef MMX
    if (GetMMX())
    {
        for (; height; height -= 1)
        {
            __2xSaILine (srcPtr, deltaPtr, srcPitch, width, dstPtr, dstPitch);
            srcPtr += srcPitch;
            dstPtr += dstPitch * 2;
            deltaPtr += srcPitch;
        }
    }
    else
#endif
    {
        for (; height; height--)
        {
            bP = (u16 *) srcPtr;
            dP = dstPtr;

            for (finish = width; finish; finish -= inc__BP)
            {
                register u32 colorA, colorB;
                u32 colorC, colorD,
                    colorE, colorF, colorG, colorH,
                    colorI, colorJ, colorK, colorL,
                    colorM, colorN, colorO, colorP;
                u32 product, product1, product2;

//-------------------------------
// Map of the pixels:     I|E F|J
//                        G|A B|K
//                        H|C D|L
//                        M|N O|P

                colorI = *(bP - Nextline - 1);
                colorE = *(bP - Nextline);
                colorF = *(bP - Nextline + 1);
                colorJ = *(bP - Nextline + 2);

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
                    if (((colorA == colorE) && (colorB == colorL)) || ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ)))
                    {
                        product = colorA;
                    }
                    else
                    {
                        product = INTERPOLATE (colorA, colorB);
                    }

                    if (((colorA == colorG) && (colorC == colorO)) || ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM)))
                    {
                        product1 = colorA;
                    }
                    else
                    {
                        product1 = INTERPOLATE (colorA, colorC);
                    }

                    product2 = colorA;
                }
                else if ((colorB == colorC) && (colorA != colorD))
                {
                    if (((colorB == colorF) && (colorA == colorH)) || ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI)))
                    {
                        product = colorB;
                    }
                    else
                    {
                        product = INTERPOLATE (colorA, colorB);
                    }

                    if (((colorC == colorH) && (colorA == colorF)) || ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI)))
                    {
                        product1 = colorC;
                    }
                    else
                    {
                        product1 = INTERPOLATE (colorA, colorC);
                    }

                    product2 = colorB;
                }
                else if ((colorA == colorD) && (colorB == colorC))
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

                        product1 = INTERPOLATE (colorA, colorC);
                        product = INTERPOLATE (colorA, colorB);

                        r += GetResult1 (colorA, colorB, colorG, colorE, colorI);
                        r += GetResult2 (colorB, colorA, colorK, colorF, colorJ);
                        r += GetResult2 (colorB, colorA, colorH, colorN, colorM);
                        r += GetResult1 (colorA, colorB, colorL, colorO, colorP);

                        if (r > 0)
                        {
                            product2 = colorA;
                        }
                        else if (r < 0)
                        {
                            product2 = colorB;
                        }
                        else
                        {
                            product2 = Q__INTERPOLATE (colorA, colorB, colorC, colorD);
                        }
                    }
                }
                else
                {
                    product2 = Q__INTERPOLATE (colorA, colorB, colorC, colorD);

                    if ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ))
                    {
                        product = colorA;
                    }
                    else if ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI))
                    {
                        product = colorB;
                    }
                    else
                    {
                        product = INTERPOLATE (colorA, colorB);
                    }

                    if ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM))
                    {
                        product1 = colorA;
                    }
                    else if ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI))
                    {
                        product1 = colorC;
                    }
                    else
                    {
                        product1 = INTERPOLATE (colorA, colorC);
                    }
                }

#ifdef BIG__ENDIAN
                product = (colorA << 16) | product ;
                product1 = (product1 << 16) | product2 ;
#else
                product = colorA | (product << 16);
                product1 = product1 | (product2 << 16);
#endif
                *((s32 *) dP) = product;
                *((u32 *) (dP + dstPitch)) = product1;

                bP += inc__BP;
                dP += sizeof (u32);
            }
            // end of for ( finish= width etc..)

            srcPtr += srcPitch;
            dstPtr += dstPitch << 1;
            deltaPtr += srcPitch;
        }
        // endof: for (height; height; height--)
    }
}

void _2xSaI32 (u8 *srcPtr, u32 srcPitch, u8 *deltaPtr, u8 *dstPtr, u32 dstPitch, int width, int height)
{
    u32 *dP;
    u32 *bP;
    u32 finish;
    u32 inc__BP = 1;
    u32 Nextline = srcPitch >> 2;

    for (; height; height--)
    {
        bP = (u32 *) srcPtr;
        dP = (u32 *) dstPtr;
        for (finish = width; finish; finish -= inc__BP)
        {
            register u32 colorA, colorB;
            u32 colorC, colorD,
                colorE, colorF, colorG, colorH,
                colorI, colorJ, colorK, colorL,
                colorM, colorN, colorO, colorP;
            u32 product, product1, product2;

//---------------------------------------
// Map of the pixels:     I|E F|J
//                        G|A B|K
//                        H|C D|L
//                        M|N O|P

            colorI = *(bP - Nextline - 1);
            colorE = *(bP - Nextline);
            colorF = *(bP - Nextline + 1);
            colorJ = *(bP - Nextline + 2);

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
                if (((colorA == colorE) && (colorB == colorL)) || ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ)))
                {
                    product = colorA;
                }
                else
                {
                    product = INTERPOLATE (colorA, colorB);
                }

                if (((colorA == colorG) && (colorC == colorO)) || ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM)))
                {
                    product1 = colorA;
                }
                else
                {
                    product1 = INTERPOLATE (colorA, colorC);
                }

                product2 = colorA;
            }
            else if ((colorB == colorC) && (colorA != colorD))
            {
                if (((colorB == colorF) && (colorA == colorH)) || ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI)))
                {
                    product = colorB;
                }
                else
                {
                    product = INTERPOLATE (colorA, colorB);
                }

                if (((colorC == colorH) && (colorA == colorF)) || ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI)))
                {
                    product1 = colorC;
                }
                else
                {
                    product1 = INTERPOLATE (colorA, colorC);
                }

                product2 = colorB;
            }
            else if ((colorA == colorD) && (colorB == colorC))
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

                    product1 = INTERPOLATE (colorA, colorC);
                    product = INTERPOLATE (colorA, colorB);
                    r += GetResult1 (colorA, colorB, colorG, colorE, colorI);
                    r += GetResult2 (colorB, colorA, colorK, colorF, colorJ);
                    r += GetResult2 (colorB, colorA, colorH, colorN, colorM);
                    r += GetResult1 (colorA, colorB, colorL, colorO, colorP);

                    if (r > 0)
                    {
                        product2 = colorA;
                    }
                    else if (r < 0)
                    {
                        product2 = colorB;
                    }
                    else
                    {
                        product2 = Q__INTERPOLATE (colorA, colorB, colorC, colorD);
                    }
                }
            }
            else
            {
                product2 = Q__INTERPOLATE (colorA, colorB, colorC, colorD);

                if ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ))
                {
                    product = colorA;
                }
                else if ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI))
                {
                    product = colorB;
                }
                else
                {
                    product = INTERPOLATE (colorA, colorB);
                }

                if ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM))
                {
                    product1 = colorA;
                }
                else if ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI))
                {
                    product1 = colorC;
                }
                else
                {
                    product1 = INTERPOLATE (colorA, colorC);
                }
            }

            *(dP) = colorA;
            *(dP + 1) = product;
            *(dP + (dstPitch >> 2)) = product1;
            *(dP + (dstPitch >> 2) + 1) = product2;

            bP += inc__BP;
            dP += 2;
        }
        // end of for ( finish= width etc..)

        srcPtr += srcPitch;
        dstPtr += dstPitch << 1;
        //deltaPtr += srcPitch;
    }
    // endof: for (height; height; height--)
}

// Not currently Used, Reserved for Future use!
//static u32 Bilinear (u32 A, u32 B, u32 x)
//{
//    unsigned long areaA, areaB;
//    unsigned long result;
//
//    if (A == B) return A;
//
//    areaB = (x >> 11) & 0x1f;     // reduce 16 bit fraction to 5 bits
//    areaA = 0x20 - areaB;
//
//    A = (A & redblueMask) | ((A & greenMask) << 16);
//    B = (B & redblueMask) | ((B & greenMask) << 16);
//
//    result = ((areaA * A) + (areaB * B)) >> 5;
//
//    return (result & redblueMask) | ((result >> 16) & greenMask);
//}
//
//static u32 Bilinear4 (u32 A, u32 B, u32 C, u32 D, u32 x, u32 y)
//{
//    unsigned long areaA, areaB, areaC, areaD;
//    unsigned long result, xy;
//
//     x = (x >> 11) & 0x1f;
//     y = (y >> 11) & 0x1f;
//    xy = (x * y) >> 5;
//
//    A = (A & redblueMask) | ((A & greenMask) << 16);
//    B = (B & redblueMask) | ((B & greenMask) << 16);
//    C = (C & redblueMask) | ((C & greenMask) << 16);
//    D = (D & redblueMask) | ((D & greenMask) << 16);
//
//    areaA = 0x20 + xy - x - y;
//    areaB = x - xy;
//    areaC = y - xy;
//    areaD = xy;
//
//    result = ((areaA * A) + (areaB * B) + (areaC * C) + (areaD * D)) >> 5;
//
//    return (result & redblueMask) | ((result >> 16) & greenMask);
//}
