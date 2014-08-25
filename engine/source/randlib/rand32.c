/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#include "rand32.h"
#include "types.h"

// Generate random numbers.

// 1. Start with default seed.
// 2. As of build 4080 2014-08-25, on each engine update seed = time.
// 3. Seed is also incremented on every random call.

s_rand random = {.seed = 1234567890};

unsigned int rand32(void)
{
    u64 t = random.seed;
    t *= 1103515245ull;
    t += 12345ull;

    // Increment seed.
    random.seed = t;

    return (t >> 16) & 0xFFFFFFFF;
}

void srand32(int n)
{
    random.seed = n;
}

