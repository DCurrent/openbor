/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef		ANIGIF_H
#define		ANIGIF_H




// Animated GIF player.

// ============================== This is it! ===============================
// Should be something like this...


#define			ANIGIF_DECODE_END		0
#define			ANIGIF_DECODE_FRAME		1
#define			ANIGIF_DECODE_RETRY		3
#define			ANIGIF_DECODE_PAL		4

#define			NO_CODE				-1


#pragma pack (1)
typedef struct
{
    char		magic[6];
    unsigned short	screenwidth, screenheight;
    unsigned char	flags;
    unsigned char	background;
    unsigned char	aspect;
} gifheaderstruct;


#pragma pack(1)
typedef struct
{
    short	left, top;
    unsigned short width, height;
    unsigned char	flags;
} gifblockstruct;

#define anigif_magic 0x464947

#pragma pack(4)
typedef struct
{
    int magic;
    struct
    {
        gifheaderstruct gif_header;
        int handle; // = -1;
        int transparent; // = -1;
        int bitdepth;
        int numcolours;
        int lastdelay;
        int code;
        u32 nextframe;
        unsigned char	*global_pal;
        unsigned char	*local_pal;
    } info[3];
    int isRGB;
    int frame;
    int done;
    s_screen *backbuffer;
    s_screen *gifbuffer[3];
} anigif_info;


//Gif file format should be always the same, so no need to use sizeof
//#if PSP || PS2 || DC
#define sizeof_gifheaderstruct 13
#define sizeof_iblock 9
//#endif


// Returns true on succes
int anigif_open(char *filename, char *packfilename, anigif_info *info);

// Decode next frame
int anigif_decode_frame(anigif_info *info);

s_screen *anigif_getbuffer(anigif_info *info);
void anigif_close(anigif_info *info);

#endif
