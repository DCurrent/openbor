/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef RAND32_H
#define RAND32_H

// *** INCLUDES ***
#include <limits.h>   // for CHAR_BIT
#include <stdlib.h>   // for abs()
#include "types.h"

extern u64 seed;

unsigned int rand32(void);
void srand32(u64);
float randf(float);

u64 getseed();
u64 rotl64(u64, unsigned int);
u64 rotr64(u64, unsigned int);

#endif




