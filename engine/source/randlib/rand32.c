/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#include "rand32.h"

s_rand random_s = { .seed = 0};

unsigned int rand32(void)
{
    unsigned int result = 0;

    // If we haven't seeded for random numbers, use time.
    if(!random_s.seed)
    {
        srand32(time(NULL));
    }

    // Get random number.
    result = rand();

    return result;
}

void srand32(unsigned long n)
{
    // Set seed.
    random_s.seed = n;

    // Apply seed to random number generator.
    srand(random_s.seed);
}

