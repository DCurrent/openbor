/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef KERNEL_PRX_H
#define KERNEL_PRX_H

#include <psploadexec_kernel.h>

int loadexec(const char *file, struct SceKernelLoadExecVSHParam *param);
int getDevkitVersion();
int getHardwareModel();

#endif
