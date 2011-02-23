/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <pspkernel.h>
#include <psploadexec_kernel.h>
#include <pspsysmem.h>
#include <pspsysmem_kernel.h>

PSP_MODULE_INFO("kernel", PSP_MODULE_KERNEL, 0, 0);
PSP_MAIN_THREAD_ATTR(0);

/******************************************************************************
	functions
******************************************************************************/

int loadexec(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	return sceKernelLoadExecVSHMs2(file, param);
}

int getDevkitVersion()
{
	return sceKernelDevkitVersion();
}

int getHardwareModel()
{
	return sceKernelGetModel();
}

int module_start(SceSize args, void *argp)
{
	return 0;
}

int module_stop()
{
	return 0;
}