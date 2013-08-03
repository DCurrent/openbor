/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/*
 * This library is used for calculating how much memory is available/used.
 * Certain platforms offer physical memory statistics, we obviously wrap
 * around those functions.  For platforms where we can't retrieve this
 * information we then calculate the estimated sizes based on a few key
 * variables and symbols.  These estimated values should tolerable.......
 */

/////////////////////////////////////////////////////////////////////////////
// Libraries

#ifdef XBOX
#include <xtl.h>
#elif WIN
#include <windows.h>
#include <psapi.h>
#elif DARWIN
#include <sys/sysctl.h>
#include <mach/task.h>
#include <mach/mach.h>
#include <mach/mach_init.h>
#elif LINUX
#include <sys/sysinfo.h>
#include <unistd.h>
#elif PSP
#include "kernel/kernel.h"
#elif GP2X
#include "gp2xport.h"
#elif OPENDINGUX
#include <stdlib.h>
#endif

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "globals.h"
#include "utils.h"
#include "ram.h"

/////////////////////////////////////////////////////////////////////////////
// Globals

static u64 systemRam = 0x00000000;

#ifndef DARWIN
#ifndef WIN
#ifndef XBOX
#ifndef LINUX
static unsigned long elfOffset = 0x00000000;
static unsigned long stackSize = 0x00000000;
#endif
#endif
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
// Symbols

#ifndef DARWIN
#ifndef WIN
#ifndef XBOX
#ifndef LINUX
#if (__GNUC__ > 3)
extern unsigned long _end;
extern unsigned long _start;
#else
extern unsigned long end;
extern unsigned long start;
#define _end end
#define _start start
#endif
#endif
#endif
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
//  Functions

u64 getFreeRam(int byte_size)
{
#if WIN || XBOX
    MEMORYSTATUS stat;
    memset(&stat, 0, sizeof(MEMORYSTATUS));
    stat.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&stat);
    return stat.dwAvailPhys / byte_size;
#elif DARWIN
    vm_size_t size;
    unsigned int count = HOST_VM_INFO_COUNT;
    vm_statistics_data_t vms;
    mach_port_t hostPort = mach_host_self();
    if(host_page_size(hostPort, &size) != KERN_SUCCESS)
    {
        return 0;
    }
    if(host_statistics(hostPort, HOST_VM_INFO, (host_info_t)&vms, &count) != KERN_SUCCESS)
    {
        return 0;
    }
    return (u64)(((vms.inactive_count + vms.free_count) * size) / byte_size);
#elif LINUX
    struct sysinfo info;
    sysinfo(&info);
    return ((u64)info.freeram) * info.mem_unit;
#elif OPENDINGUX
    FILE *file = NULL;
    const unsigned char size = 5;
    const unsigned char pos = 47;
    char result[size + 1];
    file = fopen("/proc/meminfo", "r");
    if (file == NULL)
    {
        return 0;
    }
    fseek(file, pos, SEEK_SET);
    fread(result, sizeof(char), size, file);
    fclose(file);
    result[size] = '\0';
    return (atoi(result) * 1024) / byte_size;
#elif SYMBIAN
    return GetFreeAmount();
#else
    struct mallinfo mi = mallinfo();
#ifdef _INCLUDE_MALLOC_H_
    // Standard ANSI C Implementation
    return (systemRam - (mi.arena + stackSize)) / byte_size;
#else
    return (systemRam - (mi.usmblks + stackSize)) / byte_size;
#endif
#endif
}

void setSystemRam()
{
#if WIN || XBOX
    MEMORYSTATUS stat;
    memset(&stat, 0, sizeof(MEMORYSTATUS));
    stat.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&stat);
    systemRam = stat.dwTotalPhys;
#elif DARWIN
    u64 mem;
    size_t len = sizeof(mem);
    sysctlbyname("hw.memsize", &mem, &len, NULL, 0);
    systemRam = mem;
#elif LINUX
    struct sysinfo info;
    sysinfo(&info);
    systemRam = ((u64)info.totalram) * info.mem_unit;
#elif DC
    // 16 MBytes - Memory Map:
    systemRam = 0x8d000000 - 0x8c000000;
    elfOffset = 0x8c000000;
#elif PSP
    // 24 MBytes - Memory Map:
    systemRam = 0x0A000000 - 0x08800000;
    elfOffset = 0x08800000;
    if (getHardwareModel() == 1)
    {
        systemRam += 32 * 1024 * 1024;
    }
#elif (GP2X && !WIZ)
    // 32 MBytes - Memory Map:
    systemRam = 0x02000000 - 0x00000000;
    elfOffset = 0x00000000;
    if (gp2x_init() == 2)
    {
        systemRam += 32 * 1024 * 1024;
    }
#elif (WIZ)
    // 42 MBytes - Memory Map:
    systemRam = 0x029fffff - 0x0000a2e0;
    elfOffset = 0x0000a2e0;
#elif OPENDINGUX
    // 32 MBytes - IPU Memory:
    //systemRam = 0x02000000 - 0x002C6000;
    systemRam = 0x01c8c000;//Opendingux
    elfOffset = 0x00000000;
#elif WII
    // 88 MBytes with 16 Mbytes reserved for ISO System
    systemRam = 0x817FFFFF - 0x80000000 + ((64 - 16) * 1024 * 1024);
    elfOffset = 0x80000000;
#else
    elfOffset = 0x00000000;
    stackSize = 0x00000000;
    systemRam = getFreeRam(BYTES);
#endif
#ifndef DARWIN
#ifndef WIN
#ifndef XBOX
#ifndef LINUX
#ifndef SYMBIAN
    stackSize = (int)&_end - (int)&_start + ((int)&_start - elfOffset);
#endif
#endif
#endif
#endif
#endif
    getRamStatus(BYTES);
}

u64 getSystemRam(int byte_size)
{
    return systemRam / byte_size;
}

u64 getUsedRam(int byte_size)
{
#ifdef WIN
    PROCESS_MEMORY_COUNTERS pmc;
    memset(&pmc, 0, sizeof(PROCESS_MEMORY_COUNTERS));
    pmc.cb = sizeof(pmc);
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, pmc.cb);
    return pmc.WorkingSetSize / byte_size;
#elif DARWIN
    struct task_basic_info  info;
    kern_return_t           rval = 0;
    mach_port_t             task = mach_task_self();
    mach_msg_type_number_t  tcnt = TASK_BASIC_INFO_COUNT;
    task_info_t             tptr = (task_info_t) &info;
    memset(&info, 0, sizeof(info));
    rval = task_info(task, TASK_BASIC_INFO, tptr, &tcnt);
    if (!(rval == KERN_SUCCESS))
    {
        return 0;
    }
    return info.resident_size / byte_size;
#elif LINUX
    unsigned long vm = 0;
    FILE *file = fopen("/proc/self/statm", "r");
    if (file == NULL)
    {
        return 0;
    }
    if (fscanf (file, "%lu", &vm) <= 0)
    {
        return 0;
    }
    fclose (file);
    return (vm * getpagesize() / 8) / byte_size;
#else
    return (systemRam - getFreeRam(BYTES)) / byte_size;
#endif
}

void getRamStatus(int byte_size)
{
    printf("Total Ram: %"PRIu64" Bytes\n Free Ram: %"PRIu64" Bytes\n Used Ram: %"PRIu64" Bytes\n\n",
           getSystemRam(byte_size),
           getFreeRam(byte_size),
           getUsedRam(byte_size));
}

