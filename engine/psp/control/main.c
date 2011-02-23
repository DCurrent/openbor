/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <string.h>
#include <pspkernel.h>
#include <pspctrl.h>

PSP_MODULE_INFO("control", PSP_MODULE_KERNEL, 0, 0);
PSP_MAIN_THREAD_ATTR(0);

/******************************************************************************
	local variables
******************************************************************************/

static volatile int ctrl_active;
static SceUID ctrl_thread;
static SceCtrlData ctrl_data;

/******************************************************************************
	functions
******************************************************************************/

static int ctrl_button_thread(SceSize args, void *argp)
{
	while(ctrl_active)
	{
		sceCtrlPeekBufferPositive(&ctrl_data, 1);
		sceKernelDelayThread(10 * 1000);
	}
	sceKernelExitDeleteThread(0);
	return 0;
}

void getCtrlData(SceCtrlData *data)
{
    memcpy(data, &ctrl_data, sizeof(SceCtrlData));
}

int module_start(SceSize args, void *argp)
{
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    memset(&ctrl_data, 0, sizeof(SceCtrlData));

	ctrl_active = 1;
	ctrl_thread = sceKernelCreateThread("Ctrl Button Thread",
								ctrl_button_thread,
								0x11,
								0x200,
								0,
								NULL);

	if(ctrl_thread >= 0)
		sceKernelStartThread(ctrl_thread, 0, 0);

	return 0;
}

int module_stop()
{
	if(ctrl_thread >= 0)
	{
		memset(&ctrl_data, 0, sizeof(SceCtrlData));
		sceKernelDelayThread(20 * 1000);
	}
	return 0;
}
