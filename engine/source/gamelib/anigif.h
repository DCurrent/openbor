/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef		ANIGIF_H
#define		ANIGIF_H


#pragma pack (1)

// Animated GIF player.

#define			ANIGIF_DECODE_END		0
#define			ANIGIF_DECODE_FRAME		1
#define			ANIGIF_DECODE_RETRY		3

// ============================== This is it! ===============================
// Should be something like this...


#define			ANIGIF_DECODE_END		0
#define			ANIGIF_DECODE_FRAME		1
#define			ANIGIF_DECODE_RETRY		3

#define			NO_CODE				-1



typedef struct{
	char		magic[6];
	unsigned short	screenwidth, screenheight;
		unsigned char	flags;
	unsigned char	background;
	unsigned char	aspect;
}gifheaderstruct;


typedef struct {
	short	left, top;
	unsigned short width, height;
	unsigned char	flags;
}gifblockstruct;


#if PSP || PS2 || DC
#define sizeof_gifheaderstruct 13
#define sizeof_iblock 9
#endif

typedef struct 
{
	gifheaderstruct gif_header;
	int handle; // = -1;
	int transparent; // = -1;
	int bitdepth;
	int numcolours;
	int lastdelay;
} anigif_info;



// Returns true on succes
int anigif_open(char *filename, char *packfilename, unsigned char *pal, anigif_info* info);

// Returns type of action (frame, retry or end)
int anigif_decode(s_screen * screen, int *delay, int x, int y, anigif_info* info);

void anigif_close(anigif_info* info);

#endif
