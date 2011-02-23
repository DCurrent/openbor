/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <pspkernel.h>
#include <psppower.h>
#include <pspdebug.h>

PSP_MODULE_INFO("exception", 0x1007, 1, 1);  // better not unload

PspDebugErrorHandler curr_handler;
PspDebugRegBlock *exception_regs;

void _pspDebugExceptionHandler(void);
int sceKernelRegisterDefaultExceptionHandler(void *func);
int sceKernelRegisterDefaultExceptionHandler371(void *func);

int module_start(SceSize args, void *argp)
{
   if(args != 8) return -1;
   curr_handler = (PspDebugErrorHandler)((int *)argp)[0];
   exception_regs = (PspDebugRegBlock *)((int *)argp)[1];
   if(!curr_handler || !exception_regs) return -1;

   if(sceKernelDevkitVersion() < 0x03070110)
      return sceKernelRegisterDefaultExceptionHandler((void *)_pspDebugExceptionHandler);
   else
      return sceKernelRegisterDefaultExceptionHandler371((void *)_pspDebugExceptionHandler);
}
