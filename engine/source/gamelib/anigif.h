/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef		ANIGIF_H
#define		ANIGIF_H

#include <stddef.h>



// Animated GIF player.

// ============================== This is it! ===============================
// Should be something like this...


#define			ANIGIF_DECODE_END		0
#define			ANIGIF_DECODE_FRAME		1
#define			ANIGIF_DECODE_RETRY		3
#define			ANIGIF_DECODE_PAL		4

#define			NO_CODE				-1

/* Damon V. Caskey - 2026-11-20: one RGB32 canvas per optional RGB stream. */
enum
{
    ANIGIF_CHANNEL_COUNT = 3
};


#pragma pack(1)
typedef struct
{
    char		magic[6];
    unsigned short	screenwidth, screenheight;
    unsigned char	flags;
    unsigned char	background;
    unsigned char	aspect;
} gifheaderstruct;
#pragma pack()


#pragma pack(1)
typedef struct
{
    short	left, top;
    unsigned short width, height;
    unsigned char	flags;
} gifblockstruct;
#pragma pack()

#define anigif_magic 0x464947

/*
 * Damon V. Caskey
 * 2026-11-20
 * Runtime state is not GIF wire data. Keep native pointer alignment so
 * screen/palette pointer slots are safe to pass to ownership helpers.
 */
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
        /* CPU LZW output only. Palette lookup/composition stays on the GPU. */
        unsigned char *indices;
        size_t indices_capacity;
    } info[ANIGIF_CHANNEL_COUNT];
    int isRGB;
    int frame;
    int done;
    s_screen *backbuffer;
    s_screen *gifbuffer[ANIGIF_CHANNEL_COUNT];
} anigif_info;


//Gif file format should be always the same, so no need to use sizeof
#define sizeof_gifheaderstruct 13
#define sizeof_iblock 9


// Returns true on succes
int anigif_open(char *filename, char *packfilename, anigif_info *info);

// Decode next frame
int anigif_decode_frame(anigif_info *info);

s_screen *anigif_getbuffer(anigif_info *info);
void anigif_close(anigif_info *info);

#endif
