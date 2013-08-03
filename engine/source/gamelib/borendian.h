/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef BOR_ENDIAN_H
#define BOR_ENDIAN_H

/*
	This wrapper is largely based of SDL_endian.h
	from the http://www.libsdl.org  However, it is
	now compatible with all platforms and does not
	require SDL to be installed on that platform.
	Thank You to the group of developers who created SDL!
*/

#include "types.h"

typedef s8	                SInt8;
typedef u8                  UInt8;
typedef s16                 SInt16;
typedef u16                 UInt16;
typedef s32                 SInt32;
typedef u32                 UInt32;
#ifdef __x86_64__
typedef signed long         SInt64;
typedef unsigned long       UInt64;
#else
typedef signed long long    SInt64;
typedef unsigned long long  UInt64;
#endif

#ifndef __inline__
#define __inline__ __inline
#endif

/* Use inline functions for compilers that support them, and static
   functions for those that do not.  Because these functions become
   static for compilers that do not support inline functions, this
   header should only be included in files that actually use them.
*/
#if defined(__GNUC__) && (defined(__i386__) || defined(__i586__) || defined(__i686__))&& \
   !(__GNUC__ == 2 && __GNUC_MINOR__ <= 95 /* broken gcc version */)
static __inline__ UInt16 Swap16(UInt16 x)
{
    __asm__("xchgb %b0,%h0" : "=q" (x) :  "0" (x));
    return x;
}
#elif defined(__GNUC__) && defined(__x86_64__)
static __inline__ UInt16 Swap16(UInt16 x)
{
    __asm__("xchgb %b0,%h0" : "=Q" (x) :  "0" (x));
    return x;
}
#elif defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))
static __inline__ UInt16 Swap16(UInt16 x)
{
    UInt16 result;

    __asm__("rlwimi %0,%2,8,16,23" : "=&r" (result) : "0" (x >> 8), "r" (x));
    return result;
}
#elif defined(__GNUC__) && (defined(__M68000__) || defined(__M68020__))
static __inline__ UInt16 Swap16(UInt16 x)
{
    __asm__("rorw #8,%0" : "=d" (x) :  "0" (x) : "cc");
    return x;
}
#else
static __inline__ UInt16 Swap16(UInt16 x)
{
    return((x << 8) | (x >> 8));
}
#endif

static __inline__ UInt32 Swap32(UInt32 D)
{
    return((D << 24) | ((D << 8) & 0x00FF0000) | ((D >> 8) & 0x0000FF00) | (D >> 24));
}

static __inline__ UInt64 Swap64(UInt64 val)
{
    UInt32 hi, lo;
    /* Separate into high and low 32-bit values and swap them */
    lo = (UInt32)(val & 0xFFFFFFFF);
    val >>= 32;
    hi = (UInt32)(val & 0xFFFFFFFF);
    val = Swap32(lo);
    val <<= 32;
    val |= Swap32(hi);
    return(val);
}

/* Byteswap item from the specified endianness to the native endianness */
#if (defined(__powerpc__) || defined(__ppc__) || defined(__M68000__) || defined(__M68020__))
#ifndef BOR_BIG_ENDIAN
#define BOR_BIG_ENDIAN
#endif
#endif

#ifdef BOR_BIG_ENDIAN
#define SwapLSB16(X) Swap16(X)
#define SwapLSB32(X) Swap32(X)
#define SwapLSB64(X) Swap64(X)
#define SwapMSB16(X) (X)
#define SwapMSB32(X) (X)
#define SwapMSB64(X) (X)
#else
#define SwapLSB16(X) (X)
#define SwapLSB32(X) (X)
#define SwapLSB64(X) (X)
#define SwapMSB16(X) Swap16(X)
#define SwapMSB32(X) Swap32(X)
#define SwapMSB64(X) Swap64(X)
#endif

#endif
