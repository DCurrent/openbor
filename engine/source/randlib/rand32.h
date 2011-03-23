/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef RAND32_H
#define RAND32_H

extern unsigned long seed;

unsigned int rand32(void);

void srand32(int n);

#endif




