/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <pspsdk.h>
#include <pspctrl.h>
#include <psppower.h>
#include <psptypes.h>
#include <psploadexec_kernel.h>
#include <unistd.h>
#include "menu.h"
#include "utils.h"
#include "control.h"
#include "openbor.h"
#include "packfile.h"
#include "graphics.h"
#include "kernel/kernel.h"
#include "control/control.h"

/* Define the module info section */
PSP_MODULE_INFO(VERSION_NAME, PSP_MODULE_USER, 3, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_HEAP_SIZE_MAX();

typedef void (*WriteToIO)(const char *msg, ...);
static WriteToIO pWriteToIO;

PspDebugRegBlock exception_regs;
char packfile[256] = {"bor.pak"};
char eboot[256];

void borExit(int reset)
{
	disableGraphics();
	scePowerSetClockFrequency(222, 222, 111);
	if(reset < 0)
	{
		struct SceKernelLoadExecVSHParam param;
		memset(&param, 0, sizeof(param));
		param.size = sizeof(param);
		param.argp = eboot;
		param.args = strlen(eboot)+1;
		param.key = "game";
		loadexec(eboot, &param);
	}
	else
	{
		sceKernelExitGame();
	}
}

static const char *codeTxt[32] =
{
	"Interrupt", "TLB modification", "TLB load/inst fetch", "TLB store",
	"Address load/inst fetch", "Address store", "Bus error (instr)",
	"Bus error (data)", "Syscall", "Breakpoint", "Reserved instruction",
	"Coprocessor unusable", "Arithmetic overflow", "Unknown 14",
	"Unknown 15", "Unknown 16", "Unknown 17", "Unknown 18", "Unknown 19",
	"Unknown 20", "Unknown 21", "Unknown 22", "Unknown 23", "Unknown 24",
	"Unknown 25", "Unknown 26", "Unknown 27", "Unknown 28", "Unknown 29",
	"Unknown 31"
};

static const unsigned char regName[32][5] =
{
	"zr", "at", "v0", "v1", "a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

void ExceptionHandler(PspDebugRegBlock * regs)
{
	int i;
	int loop = 0;
	extern unsigned long _ftext;

	pspDebugScreenInit();
	pspDebugScreenSetBackColor(0x00FF0000);
	pspDebugScreenSetTextColor(0xFFFFFFFF);
	pspDebugScreenClear();

WRITE_EXCEPTION:
	if(!loop) pWriteToIO = pspDebugScreenPrintf;
	else pWriteToIO = writeToLogFile;

	pWriteToIO("\n I'm sorry to inform your PSP has crashed. The exception has\n");
	pWriteToIO(" been logged to a file. Please include OpenBOR.ELF within the\n");
	pWriteToIO(" Modules directory and the log file generated to www.LavaLit.com\n\n\n\n");
	pWriteToIO(" Exception Details:\n\n\n");
	pWriteToIO(" Exception - %s / %s.text + %X\n", codeTxt[(regs->cause >> 2) & 31], module_info.modname, regs->epc-(int)&_ftext);
	pWriteToIO(" EPC       - %08X\n", (int)regs->epc);
	pWriteToIO(" REPC      - %08X\n", (int)regs->epc-(int)&_ftext);
	pWriteToIO(" Cause     - %08X\n", (int)regs->cause);
	pWriteToIO(" Status    - %08X\n", (int)regs->status);
	pWriteToIO(" BadVAddr  - %08X\n\n\n", (int)regs->badvaddr);

	for(i=0; i<32; i+=4) pWriteToIO(" %s:%08X %s:%08X %s:%08X %s:%08X\n", regName[i], (int)regs->r[i], regName[i+1], (int)regs->r[i+1], regName[i+2], (int)regs->r[i+2], regName[i+3], (int)regs->r[i+3]);

	loop++;
	if(loop < 2) goto WRITE_EXCEPTION;

	pspDebugScreenPrintf("\n\n\n\n\n");
	pspDebugScreenPrintf(" Press Home key to exit.\n");
	pspDebugScreenPrintf(" Press Select key to capture screen.");
	while(1)
	{
		SceCtrlData data;
		getCtrlData(&data);
		if(data.Buttons & PSP_CTRL_HOME) sceKernelExitGame();
		if(data.Buttons & PSP_CTRL_SELECT) screenshot(NULL, NULL, 0);
	}
}

void initExceptionHandler()
{
   SceKernelLMOption option;
   int args[2], fd, modid;

   memset(&option, 0, sizeof(option));
   option.size = sizeof(option);
   option.mpidtext = PSP_MEMORY_PARTITION_KERNEL;
   option.mpiddata = PSP_MEMORY_PARTITION_KERNEL;
   option.position = 0;
   option.access = 1;

   if((modid = sceKernelLoadModule("/Modules/exception.prx", 0, &option)) >= 0)
   {
	  args[0] = (int)ExceptionHandler;
	  args[1] = (int)&exception_regs;
	  sceKernelStartModule(modid, 8, args, &fd, NULL);
   }
}

int main(int argc, char *argv[])
{
	char cwd[256];
	int status = 0;

	scePowerSetClockFrequency(333, 333, 166);
	initExceptionHandler();
	strncpy(eboot, argv[0], strlen(argv[0]));

	if((status = pspSdkLoadStartModule("/Modules/kernel.prx", PSP_MEMORY_PARTITION_KERNEL)) < 0) goto error_loading_prx_modules;
	if((status = pspSdkLoadStartModule("/Modules/control.prx", PSP_MEMORY_PARTITION_KERNEL)) < 0) goto error_loading_prx_modules;
	if(getHardwareModel()==1 && (status = pspSdkLoadStartModule("/Modules/dvemgr.prx", PSP_MEMORY_PARTITION_KERNEL)) < 0) goto error_loading_prx_modules;

	setSystemRam();
	packfile_mode(0);
	loadsettings();
	setGraphicsTVOverScan(savedata.overscan[0], savedata.overscan[1], savedata.overscan[2], savedata.overscan[3]);
	initGraphics(savedata.usetv, PIXEL_32);

	dirExists("Saves", 1);
	dirExists("Paks", 1);
	dirExists("Images", 1);
	dirExists("Logs", 1);

	getcwd(cwd, 256);
	menu(cwd);
	openborMain(argc, argv);
	borExit(0);
	return 0;

error_loading_prx_modules:
	writeToLogFile("PRX Modules failed with %x\n", status);
	sceKernelDelayThread(5*1000000);
	return 0;
}
