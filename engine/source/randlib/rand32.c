/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#include "rand32.h"

u64 seed = 1234567890;

unsigned int rand32(void)
{
    u64 t = seed, tl, tr;

    t *= 1103515245ull;
    t += 12345ull;

    tl = tr = t;
    t = rotr64(t,4);

    // less significative part
    tr = (((~tl)>>16)^(0xAC05)) & 0xFFFF;

    // more significative part
    tl = ((tr&0xFFFF)&0x1234)<<16;
    tl = (tl|tr) & 0xFFFFFFFF;
    tl = rotl64(tl,8);

    t *= tl;

    seed = t;

    return ( (t >> 16) & 0xFFFFFFFF );
}

void srand32(u64 n)
{
    seed = n;
}

float randf(float max)
{
    float f;

    if(max == 0)
    {
        return 0;
    }
    f = (float)(rand32() % 10000);
    f /= (10000 / max);

    return f;
}

/* --- RAND UTILS --- */

u64 getseed()
{
    return seed;
}

u64 rotl64(u64 n, unsigned int c)
{
  const unsigned int mask = (CHAR_BIT*sizeof(n)-1);

  c &= mask;
  return (n<<c) | (n>>( (-c)&mask ));
}

u64 rotr64(u64 n, unsigned int c)
{
  const unsigned int mask = (CHAR_BIT*sizeof(n)-1);

  c &= mask;
  return (n>>c) | (n<<( (-c)&mask ));
}


