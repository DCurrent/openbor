/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef PSP_H
#define PSP_H

#include "globals.h"

void openborMain(int argc, char** argv);
void borExit(int reset);

extern char packfile[MAX_FILENAME_LEN];

#endif
