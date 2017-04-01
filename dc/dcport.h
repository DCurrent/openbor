/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef DCPORT_H
#define DCPORT_H

/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <errno.h>

#include "kos.h"
#include "ram.h"
#include "gdrom.h"
#include "globals.h"

/////////////////////////////////////////////////////////////////////////////

extern char packfile[128];
extern int cd_lba;

/////////////////////////////////////////////////////////////////////////////

unsigned readmsb32(const unsigned char *src);
void openborMain(int main, char** argv);
void borExit(int reset);

/////////////////////////////////////////////////////////////////////////////

#endif
