/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "utils.h"
#include "sdlport.h"
#include "gp2xport.h"
#include <fcntl.h>
#include <linux/fb.h>
#include <stropts.h>

#define SYS_CLK_FREQ 7372800
#define BLOCKSIZE 1024


static int gp2x_mem;
static int gp2x_mixer;
#ifndef WIZ
volatile static uint32_t* gp2x_memregl;
volatile static uint16_t* gp2x_memregs;
int Uppermemfd;
void *UpperMem;
int TakenSize[0x2000000 / BLOCKSIZE];
#endif

#define SetTaken(Start, Size) TakenSize[(Start - 0x2000000) / BLOCKSIZE] = (Size - 1) / BLOCKSIZE + 1

/* Call this MMU Hack kernel module after doing mmap, and before doing memset */
int mmuhack()
{
#ifndef WIZ
	int mmufd;
	system("/sbin/rmmod mmuhack");
    system("/sbin/insmod mmuhack.o");
    mmufd = open("/dev/mmuhack", O_RDWR);
    if(mmufd < 0) return 0;
    close(mmufd);
#endif
    return 1;
}

/* Unload MMU Hack kernel module after closing all memory devices */
void mmuunhack(void)
{
#ifndef WIZ  
    system("/sbin/rmmod mmuhack");
#endif
}

void * UpperMalloc(int size)
{
#ifndef WIZ  
	int i = 0;
	int j = 1;
ReDo:
	for(; TakenSize[i]; i += TakenSize[i]);
	if(i >= 0x2000000 / BLOCKSIZE)
	{
		writeToLogFile("UpperMalloc out of mem!");
		return NULL;
	}
	int BSize = (size - 1) / BLOCKSIZE + 1;
	for(j = 1; j < BSize; j++)
	{
		if(TakenSize[i + j])
		{
			i += j;
			goto ReDo; //OMG Goto, kill me.
		}
    }

	TakenSize[i] = BSize;
	void* mem = ((char*)UpperMem) + i * BLOCKSIZE;
	memset(mem, 0, size);
	return mem;
#else
	return NULL;
#endif
}

//Releases UpperMalloced memory
void UpperFree(void* mem)
{
#ifndef WIZ  
	int i = (((int)mem) - ((int)UpperMem));
	if(i < 0 || i >= 0x2000000) writeToLogFile("UpperFree of not UpperMalloced mem: %p\n", mem);
    else
	{
		if(i % BLOCKSIZE) writeToLogFile("delete error: %p\n", mem);
		TakenSize[i / BLOCKSIZE] = 0;
	}
#endif	
}

//Returns the size of a UpperMalloced block.
int GetUpperSize(void* mem)
{
#ifndef WIZ
	int i = (((int)mem) - ((int)UpperMem));
	if(i < 0 || i >= 0x2000000)
	{
		writeToLogFile("GetUpperSize of not UpperMalloced mem: %p\n", mem);
		return -1;
	}
	return TakenSize[i / BLOCKSIZE] * BLOCKSIZE;
#else
	return 0;
#endif	
}

#ifndef WIZ
int InitMemPool()
{
	int uRam = 1;
	//Try to apply MMU hack.
	int mmufd = open("/dev/mmuhack", O_RDWR);
	if(mmufd < 0)
	{
		system("/sbin/insmod mmuhack.o");
		mmufd = open("/dev/mmuhack", O_RDWR);
	}
	if(mmufd < 0)
	{
		writeToLogFile("MMU hack failed");
		return 0;
	}
	else close(mmufd);

	Uppermemfd = open("/dev/mem", O_RDWR);
	if((UpperMem = mmap(0, 0x2000000, PROT_READ | PROT_WRITE, MAP_SHARED, Uppermemfd, 0x2000000))) uRam = 2;

	memset(TakenSize, 0, sizeof(TakenSize));

	SetTaken(0x3000000, 0x80000); // Video decoder (you could overwrite this, but if you
                                  // don't need the memory then be nice and don't)
	SetTaken(0x3101000, 153600);  // Primary frame buffer
	SetTaken(0x3381000, 153600);  // Secondary frame buffer (if you don't use it, uncomment)
	SetTaken(0x3600000, 0x8000);  // Sound buffer

	return uRam;
}
#endif

#ifndef WIZ
void DestroyMemPool()
{
	close(Uppermemfd);
	UpperMem = NULL;
}
#endif	

void gp2x_end()
{
	close(gp2x_mixer);
#ifndef WIZ	
	close(gp2x_mem);
	DestroyMemPool();
	mmuunhack();
#endif
}

int gp2x_init()
{
#ifndef WIZ
	int gp2x_type;
#endif
	gp2x_mem = open("/dev/mem", O_RDWR);
	gp2x_mixer = open("/dev/mixer", O_RDWR);
#ifndef WIZ
	gp2x_memregl = (uint32_t*)mmap(0, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, gp2x_mem, 0xc0000000);
	gp2x_memregs = (uint16_t*)gp2x_memregl;
	gp2x_type = InitMemPool();
	if(!gp2x_type || !gp2x_mem || !gp2x_mixer || !gp2x_memregl || !gp2x_memregs)
	{
		printf("GP2X Failed to Init!!!!\n");
		borExit(0);
	}
	return gp2x_type;
#else
	return 0;
#endif
}

void gp2x_set_clock(int mhz)
{
#ifndef WIZ
	mhz *= 1000000;
	unsigned pdiv = 3;
	unsigned mdiv = (mhz * pdiv) / SYS_CLK_FREQ;
	mdiv = ((mdiv - 8) << 8) & 0xff00;
	pdiv = ((pdiv - 2) << 2) & 0xfc;
	unsigned scale = 3;
	unsigned v = mdiv | pdiv | scale;
	gp2x_memregs[0x0910 >> 1] = v;
#endif	
}

void gp2x_sound_set_volume(int l, int r)
{
	int vol = (((l * 0x50) / 100) << 8) | ((r * 0x50) / 100);
	ioctl(gp2x_mixer, SOUND_MIXER_WRITE_PCM, &vol);
}

void gp2x_video_wait_vsync()
{
#ifndef WIZ
	while (gp2x_memregs[0x1182 >> 1] & (1 << 4));
#endif	
}
