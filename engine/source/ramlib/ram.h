/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef RAM_H
#define RAM_H

#define BYTES  (1)
#define KBYTES (1024)
#define MBYTES (1024*1024)

#include "types.h"

void setSystemRam();
u64 getSystemRam(int);
u64 getFreeRam(int);
u64 getUsedRam(int);
void getRamStatus(int);

#endif
