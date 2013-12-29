/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// Functions to load GIF, PCX and BMP files.
// Last update: 26-jan-2003
// Now loading to screens or bitmaps,
// creating them on-the-fly if necessary.

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "types.h"
#include "borendian.h"
#include "bitmap.h"
#include "screen.h"
#include "packfile.h"
#include "png.h"

#ifndef DC
#pragma pack (1)
#endif

// ============================== Globals ===============================

static int handle = -1;
static int res[2] = {0, 0};	// Resolution of opened image

// ============================== BMP loading ===============================

typedef struct
{
    unsigned short	bm;
    int		        filesize;
    int		        reserved;
    int		        picstart;
    int		        headersize;
    int		        xsize;
    int		        ysize;
    unsigned short	numplanes;
    unsigned short	bpp;
    int		        compression;
    int		        picsize;
    int		        hres;
    int		        vres;
    int		        numcolors_used;
    int		        numcolors_important;
} s_bmpheader;

static s_bmpheader bmp_header;

#if DC || PS2 || GP2X || SYMBIAN
static void build_bmp_header(s_bmpheader *h, const unsigned char *s)
{

    h->bm          = readlsb32(s + 0x00);
    h->filesize    = readlsb32(s + 0x02);
    h->reserved    = readlsb32(s + 0x06);
    h->picstart    = readlsb32(s + 0x0A);
    h->headersize  = readlsb32(s + 0x0E);
    h->xsize       = readlsb32(s + 0x12);
    h->ysize       = readlsb32(s + 0x16);
    h->numplanes   = readlsb32(s + 0x1A);
    h->bpp         = readlsb32(s + 0x1C);
    h->compression = readlsb32(s + 0x1E);
    h->picsize     = readlsb32(s + 0x22);
    h->hres        = readlsb32(s + 0x26);
    h->vres        = readlsb32(s + 0x2A);
    h->numcolors_used      = readlsb32(s + 0x2E);
    h->numcolors_important = readlsb32(s + 0x32);
}
#endif

// Open a BMP stream
static int openbmp(char *filename, char *packfilename)
{

#if DC || PS2 || GP2X || SYMBIAN
    unsigned char mybmpheader[0x36];
#endif

    if((handle = openpackfile(filename, packfilename)) == -1)
    {
        return 0;
    }
#if DC || PS2 || GP2X || SYMBIAN
    if(readpackfile(handle, &mybmpheader, 0x36) != 0x36)
    {
#else
    if(readpackfile(handle, &bmp_header, sizeof(s_bmpheader)) != sizeof(s_bmpheader))
    {
#endif
        closepackfile(handle);
        return 0;
    }

#if DC || PS2 || GP2X || SYMBIAN
    build_bmp_header(&bmp_header, mybmpheader);
#else
    bmp_header.bm = SwapLSB16(bmp_header.bm);
    bmp_header.numplanes = SwapLSB16(bmp_header.numplanes);
    bmp_header.bpp = SwapLSB16(bmp_header.bpp);

    bmp_header.filesize = SwapLSB32(bmp_header.filesize);
    bmp_header.reserved = SwapLSB32(bmp_header.reserved);
    bmp_header.picstart = SwapLSB32(bmp_header.picstart);
    bmp_header.headersize = SwapLSB32(bmp_header.headersize);
    bmp_header.xsize = SwapLSB32(bmp_header.xsize);
    bmp_header.ysize = SwapLSB32(bmp_header.ysize);
    bmp_header.filesize = SwapLSB32(bmp_header.filesize);

    bmp_header.compression = SwapLSB32(bmp_header.compression);
    bmp_header.picsize = SwapLSB32(bmp_header.picsize);
    bmp_header.hres = SwapLSB32(bmp_header.hres);
    bmp_header.vres = SwapLSB32(bmp_header.vres);
    bmp_header.numcolors_used = SwapLSB32(bmp_header.numcolors_used);
    bmp_header.numcolors_important = SwapLSB32(bmp_header.numcolors_important);
#endif

    if(bmp_header.bm != 0x4D42 || bmp_header.bpp != 8 || bmp_header.compression)
    {
        closepackfile(handle);
        return 0;
    }
    res[0] = bmp_header.xsize;
    res[1] = bmp_header.ysize;
    return 1;
}

// Read data from the bitmap file
static int readbmp(unsigned char *buf, unsigned char *pal, int maxwidth, int maxheight)
{

    unsigned char *linebuffer;
    int y, s, d;
    int pb = PAL_BYTES;

    if(buf)
    {
        y = 0;
        while(y < maxheight && y < bmp_header.ysize)
        {
            linebuffer = buf + y * maxwidth;
            seekpackfile(handle, bmp_header.picstart + ((bmp_header.ysize - y - 1)*bmp_header.xsize), SEEK_SET);
            readpackfile(handle, linebuffer, bmp_header.xsize);
            ++y;
        }
    }

    if(pal && (linebuffer = (unsigned char *)malloc(1024)))
    {
        seekpackfile(handle, bmp_header.picstart - 1024, SEEK_SET);
        readpackfile(handle, linebuffer, 1024);
        if(pb == 512) // 16bit 565
        {
            for(s = 0, d = 0; s < 1024; s += 4, d += 2)
            {
                *(unsigned short *)(pal + d) = colour16(linebuffer[s + 2], linebuffer[s + 1], linebuffer[s]);
            }
        }
        else if(pb == 768) // 24bit palette, RGBA-BGR
        {
            for(s = 0, d = 0; s < 1024; s += 4, d += 3)
            {
                pal[d] = linebuffer[s + 2];
                pal[d + 1] = linebuffer[s + 1];
                pal[d + 2] = linebuffer[s];
            }
        }
        else if(pb == 1024)
        {
            for(s = 0, d = 0; s < 1024; s += 4, d += 4)
            {
                *(unsigned *)(pal + d) = colour32(linebuffer[s + 2], linebuffer[s + 1], linebuffer[s]);
            }
        }
        free(linebuffer);
        linebuffer = NULL;
    }

    return 1;
}

//static void closebmp(){
//	closepackfile(handle);
//	handle = -1;
//}

//============================ PNG, use libpng =============================
static int png_height = 0;
static png_structp png_ptr = NULL;
static png_infop info_ptr = NULL;
static png_bytep *row_pointers = NULL;

static void png_read_fn(png_structp pngp, png_bytep outp, png_size_t size)
{
    readpackfile(*(int *)(png_get_io_ptr(pngp)), outp, size);
}

static void png_read_destroy_all()
{
    if(png_ptr)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    }
    info_ptr = NULL;
    png_ptr = NULL;
}

static void closepng()
{
    int y;
    png_read_destroy_all();
    if(row_pointers)
    {
        for (y = 0; y < png_height; y++)
        {
            free(row_pointers[y]);
            row_pointers[y] = NULL;
        }
        free(row_pointers);
        row_pointers = NULL;
    }
    png_height = 0;
    if(handle >= 0)
    {
        closepackfile(handle);
    }
    handle = -1;
}

static int openpng(char *filename, char *packfilename)
{
    unsigned char header[8];    // 8 is the maximum size that can be checked
    int y;

    if((handle = openpackfile(filename, packfilename)) == -1)
    {
        goto openpng_abort;
    }

    if(readpackfile(handle, header, 8) != 8)
    {
        goto openpng_abort;
    }

    if (png_sig_cmp(header, 0, 8))
    {
        goto openpng_abort;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
    {
        goto openpng_abort;
    }

    //UT: use customized file read function here, because we use pak file methods instead of stdio
    png_set_read_fn(png_ptr, &handle, png_read_fn);

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        goto openpng_abort;
    }

    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    //UT: not formal here, but just read what we need since we use only 8bit png for now
    res[0] = png_get_image_width(png_ptr, info_ptr);
    png_height = res[1] = png_get_image_height(png_ptr, info_ptr);
    // should only be a 8bit image by now
    if (png_get_bit_depth(png_ptr, info_ptr) != 8)
    {
        goto openpng_abort;
    }

    png_read_update_info(png_ptr, info_ptr);

    row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * png_height);
    for (y = 0; y < png_height; y++)
    {
        row_pointers[y] = (png_byte *) malloc(png_get_rowbytes(png_ptr, info_ptr));
    }

    png_read_image(png_ptr, row_pointers);
    return 1;
openpng_abort:
    closepng();
    return 0;
}

static int readpng(unsigned char *buf, unsigned char *pal, int maxwidth, int maxheight)
{
    int i, j, cw, ch;
    png_colorp png_pal_ptr = 0;
    int png_pal_num = 0;
    int pb = PAL_BYTES;

    cw = res[0] > maxwidth ? maxwidth : res[0];
    ch = res[1] > maxheight ? maxheight : res[1];
    if(buf)
    {
        for(i = 0; i < ch; i++)
        {
            memcpy(buf + (maxwidth * i), row_pointers[i], cw);
        }
    }
    if(pal)
    {
        if(png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY)
        {
            // set palette for grayscale images
            for(i = 0; i < 256; i++)
            {
                pal[i * 3] = pal[i * 3 + 1] = pal[i * 3 + 2] = i;
            }
            return 1;
        }
        else if(png_get_PLTE(png_ptr, info_ptr, &png_pal_ptr, &png_pal_num) != PNG_INFO_PLTE ||
                png_pal_ptr == NULL)
        {
            return 0;
        }

        png_pal_ptr[0].red = png_pal_ptr[0].green = png_pal_ptr[0].blue = 0;
        if(pb == 512) // 16bit 565
        {
            for(i = 0, j = 0; i < 512 && j < png_pal_num; i += 2, j++)
            {
                *(unsigned short *)(pal + i) = colour16(png_pal_ptr[j].red, png_pal_ptr[j].green, png_pal_ptr[j].blue);
            }
        }
        else if(pb == 768) // 24bit
        {
            for(i = 0; i < png_pal_num; i++)
            {
                pal[i * 3] = png_pal_ptr[i].red;
                pal[i * 3 + 1] = png_pal_ptr[i].green;
                pal[i * 3 + 2] = png_pal_ptr[i].blue;
            }
        }
        else if(pb == 1024) // 32bit
        {

            for(i = 0, j = 0; i < 1024 && j < png_pal_num; i += 4, j++)
            {
                *(unsigned *)(pal + i) = colour32(png_pal_ptr[j].red, png_pal_ptr[j].green, png_pal_ptr[j].blue);
            }
        }
    }
    return 1;
}


// ============================== GIF loading ===============================


typedef struct
{
    char		    magic[6];
    unsigned short	screenwidth, screenheight;
    unsigned char	flags;
    unsigned char	background;
    unsigned char	aspect;
} gifheaderstruct;

#if DC || PS2 || GP2X || SYMBIAN
#define sizeof_gifheaderstruct 13
#endif

typedef struct
{
    unsigned short   left, top, width, height;
    unsigned char    flags;
} gifblockstruct;

#if DC || PS2 || GP2X || SYMBIAN
#define sizeof_iblock 9
#endif

static gifheaderstruct gif_header;

static unsigned char readbyte(int handle)
{
    unsigned char c = 0;
    readpackfile(handle, &c, 1);
    return c;
}

#define NO_CODE -1

static int decodegifblock(int handle, unsigned char *buf, int width, int height, unsigned char bits, gifblockstruct *gb)
{
    short bits2;
    short codesize;
    short codesize2;
    short nextcode;
    short thiscode;
    short oldtoken;
    short currentcode;
    short oldcode;
    short bitsleft;
    short blocksize;
    int line = 0;
    int byte = gb->left;
    int pass = 0;

    unsigned char *p;
    unsigned char *u;

    unsigned char *q;
    unsigned char b[255];
    unsigned char *linebuffer;

    static unsigned char firstcodestack[4096];
    static unsigned char lastcodestack[4096];
    static short codestack[4096];

    static short wordmasktable[] = {    0x0000, 0x0001, 0x0003, 0x0007,
                                        0x000f, 0x001f, 0x003f, 0x007f,
                                        0x00ff, 0x01ff, 0x03ff, 0x07ff,
                                        0x0fff, 0x1fff, 0x3fff, 0x7fff
                                   };

    static short inctable[] = { 8, 8, 4, 2, 0 };
    static short startable[] = { 0, 4, 2, 1, 0 };

    p = q = b;
    bitsleft = 8;

    if (bits < 2 || bits > 8)
    {
        return 0;    // Bad symbol
    }
    bits2 = 1 << bits;
    nextcode = bits2 + 2;
    codesize2 = 1 << (codesize = bits + 1);
    oldcode = oldtoken = NO_CODE;

    linebuffer = buf + (gb->top * width);

    // loop until something breaks
    for(;;)
    {
        if(bitsleft == 8)
        {
            if(++p >= q && (((blocksize = (unsigned char)readbyte(handle)) < 1) ||
                            (q = (p = b) + readpackfile(handle, b, blocksize)) < (b + blocksize)))
            {
                return 0;        // Unexpected EOF
            }
            bitsleft = 0;
        }
        thiscode = *p;
        if((currentcode = (codesize + bitsleft)) <= 8)
        {
            *p >>= codesize;
            bitsleft = currentcode;
        }
        else
        {
            if(++p >= q && (((blocksize = (unsigned char)readbyte(handle)) < 1) ||
                            (q = (p = b) + readpackfile(handle, b, blocksize)) < (b + blocksize)))
            {
                return 0;        // Unexpected EOF
            }

            thiscode |= *p << (8 - bitsleft);
            if(currentcode <= 16)
            {
                *p >>= (bitsleft = currentcode - 8);
            }
            else
            {
                if(++p >= q && (((blocksize = (unsigned char)readbyte(handle)) < 1) ||
                                (q = (p = b) + readpackfile(handle, b, blocksize)) < (b + blocksize)))
                {
                    return 0;    // Unexpected EOF
                }

                thiscode |= *p << (16 - bitsleft);
                *p >>= (bitsleft = currentcode - 16);
            }
        }
        thiscode &= wordmasktable[codesize];
        currentcode = thiscode;

        if(thiscode == (bits2 + 1))
        {
            break;
        }
        if(thiscode > nextcode)
        {
            return 0;    // Bad code
        }

        if(thiscode == bits2)
        {
            nextcode = bits2 + 2;
            codesize2 = 1 << (codesize = (bits + 1));
            oldtoken = oldcode = NO_CODE;
            continue;
        }

        u = firstcodestack;

        if(thiscode == nextcode)
        {
            if(oldcode == NO_CODE)
            {
                return 0;    // Bad code
            }
            *u++ = oldtoken;
            thiscode = oldcode;
        }

        while(thiscode >= bits2)
        {
            *u++ = lastcodestack [thiscode];
            thiscode = codestack[thiscode];
        }

        oldtoken = thiscode;
        do
        {
            if(byte < width && line < (height - gb->top))
            {
                linebuffer[byte] = thiscode;
            }
            byte++;
            if(byte >= gb->left + gb->width)
            {
                byte = gb->left;
                // check for interlaced image
                if(gb->flags & 0x40)
                {
                    line += inctable[pass];
                    if(line >= gb->height)
                    {
                        line = startable[++pass];
                    }
                }
                else
                {
                    ++line;
                }
                linebuffer = buf + (width * (gb->top + line));
            }
            if (u <= firstcodestack)
            {
                break;
            }
            thiscode = *--u;
        }
        while(1);

        if(nextcode < 4096 && oldcode != NO_CODE)
        {
            codestack[nextcode] = oldcode;
            lastcodestack[nextcode] = oldtoken;
            if(++nextcode >= codesize2 && codesize < 12)
            {
                codesize2 = 1 << ++codesize;
            }
        }
        oldcode = currentcode;
    }
    return 1;
}

static void passgifblock(int handle)
{
    int len;

    // Discard extension function code
    len = readbyte(handle);
    // Skip all contained blocks
    while((len = readbyte(handle)) != 0)
    {
        seekpackfile(handle, len, SEEK_CUR);
    }
}

static int opengif(char *filename, char *packfilename)
{

    if((handle = openpackfile(filename, packfilename)) == -1)
    {
        return 0;
    }

#if DC || PS2 || GP2X || SYMBIAN
    if(readpackfile(handle, &gif_header, sizeof_gifheaderstruct) != sizeof_gifheaderstruct)
    {
#else
    if(readpackfile(handle, &gif_header, sizeof(gifheaderstruct)) != sizeof(gifheaderstruct))
    {
#endif
        closepackfile(handle);
        return 0;
    }
    if(gif_header.magic[0] != 'G' || gif_header.magic[1] != 'I' || gif_header.magic[2] != 'F')
    {
        // Not a GIF file!
        closepackfile(handle);
        return 0;
    }

    gif_header.screenwidth = SwapLSB16(gif_header.screenwidth);
    gif_header.screenheight = SwapLSB16(gif_header.screenheight);

    res[0] = gif_header.screenwidth;
    res[1] = gif_header.screenheight;

    return 1;
}

static int readgif(unsigned char *buf, unsigned char *pal, int maxwidth, int maxheight)
{

    gifblockstruct iblock;
    int bitdepth;
    int numcolours;
    int i, j;
    int done = 0;
    unsigned char *pbuf;
    unsigned char c;
    int pb = PAL_BYTES;


    bitdepth = (gif_header.flags & 7) + 1;
    numcolours = (1 << bitdepth);


    // get palette if present and if wanted
    if(gif_header.flags & 0x80)
    {
        if(pal)
        {
            if(pb == 512) // 16bit 565
            {
                pbuf = malloc(768);
                if(readpackfile(handle, pbuf, numcolours * 3) != numcolours * 3)
                {
                    free(pbuf);
                    pbuf = NULL;
                    return 0;
                }
                for(i = 0, j = 0; i < 512; i += 2, j += 3)
                {
                    *(unsigned short *)(pal + i) = colour16(pbuf[j], pbuf[j + 1], pbuf[j + 2]);
                }
                free(pbuf);
                pbuf = NULL;
            }
            else if(pb == 768) // 24bit
            {
                if(readpackfile(handle, pal, numcolours * 3) != numcolours * 3)
                {
                    return 0;
                }
            }
            else if(pb == 1024) // 32bit
            {
                pbuf = malloc(768);
                if(readpackfile(handle, pbuf, numcolours * 3) != numcolours * 3)
                {
                    free(pbuf);
                    pbuf = NULL;
                    return 0;
                }
                for(i = 0, j = 0; i < 1024; i += 4, j += 3)
                {
                    *(unsigned *)(pal + i) = colour32(pbuf[j], pbuf[j + 1], pbuf[j + 2]);
                }
                free(pbuf);
                pbuf = NULL;
            }
        }
        else
        {
            seekpackfile(handle, numcolours * 3, SEEK_CUR);
        }
    }

    if(!buf)
    {
        return 1;
    }

    // step through the blocks while(c==',' || c=='!' || c==0)
    while(!done)
    {
        if(readpackfile(handle, &c, 1) != 1)
        {
            break;
        }
        switch(c)
        {
        case ',':        // An image block

#if DC || PS2 || GP2X || SYMBIAN
            if(readpackfile(handle, &iblock, sizeof_iblock) != sizeof_iblock)
            {
#else
            if(readpackfile(handle, &iblock, sizeof(iblock)) != sizeof(iblock))
            {
#endif
                return 0;
            }

            iblock.left = SwapLSB16(iblock.left);
            iblock.top = SwapLSB16(iblock.top);
            iblock.width = SwapLSB16(iblock.width);
            iblock.height = SwapLSB16(iblock.height);

            // get local palette if present and wanted
            if((iblock.flags & 0x80) && pal)
            {
                if(pal)
                {
                    i = 3 * (1 << ((iblock.flags & 0x0007) + 1));
                    if(readpackfile(handle, pal, i) != numcolours)
                    {
                        return 0;
                    }
                }
                else
                {
                    seekpackfile(handle, numcolours * 3, SEEK_CUR);
                }
            }

            // get the initial LZW code bits
            if(readpackfile(handle, &c, 1) != 1)
            {
                return 0;
            }
            if(c < 2 || c > 8)
            {
                return 0;
            }
            if(!decodegifblock(handle, buf, maxwidth, maxheight, c, &iblock))
            {
                return 0;
            }
            break;
        case '!':
            // Extension block, read past it
            passgifblock(handle);
            break;
        case 0:
            // Isn't this an EOF?
            break;
        default:
            done = 1;
        }
    }
    return 1;
}

// ============================== PCX loading ===============================

typedef struct
{
    char            manufacturer;
    char            version;
    char            encoding;
    char            bitspp;
    unsigned short  xmin, ymin;
    unsigned short  xmax, ymax;
    unsigned short  hres, vres;
    char            unused[48];
    char            reserved;
    char            colorplanes;
    unsigned short  bytesperline;
    unsigned short  paltype;
    char            unused2[58];
} s_pcxheader;

static s_pcxheader pcx_header;

static int openpcx(char *filename, char *packname)
{
    if((handle = openpackfile(filename, packname)) == -1)
    {
        return 0;
    }
    if(readpackfile(handle, &pcx_header, 128) != 128)
    {
        closepackfile(handle);
        return 0;
    }

    pcx_header.xmin = SwapLSB16(pcx_header.xmin);
    pcx_header.ymin = SwapLSB16(pcx_header.ymin);
    pcx_header.xmax = SwapLSB16(pcx_header.xmax);
    pcx_header.ymax = SwapLSB16(pcx_header.ymax);
    pcx_header.hres = SwapLSB16(pcx_header.hres);
    pcx_header.vres = SwapLSB16(pcx_header.vres);
    pcx_header.bytesperline = SwapLSB16(pcx_header.bytesperline);
    pcx_header.paltype = SwapLSB16(pcx_header.paltype);

    if(pcx_header.colorplanes == 3)
    {
        closepackfile(handle);
        return 0;
    }

    res[0] = pcx_header.xmax;
    res[1] = pcx_header.ymax;

    return 1;
}

static int readpcx(unsigned char *buf, unsigned char *pal, int maxwidth, int maxheight)
{

    unsigned char *codebuffer;
    unsigned int i, j;
    unsigned x, y;
    unsigned spos;        // Searchpos (file)
    unsigned cpos;        // codepos
    unsigned dpos;        // Destination
    unsigned char repbyte;
    unsigned char *pbuf;
    int pb = PAL_BYTES;

    if(buf)
    {

        if(!(codebuffer = (unsigned char *)malloc(64000)))
        {
            return 0;
        }

        x = 0;
        y = 0;
        spos = 0;
        dpos = 0;

        while(y <= pcx_header.ymax && y < maxheight)
        {

            seekpackfile(handle, 128 + spos, SEEK_SET);

            if((readpackfile(handle, codebuffer, 64000)) == -1)
            {
                free(codebuffer);
                codebuffer = NULL;
                return 0;
            }

            cpos = 0;

            while(cpos < 63990 && y <= pcx_header.ymax && y < maxheight)
            {
                if(codebuffer[cpos] > 192)
                {
                    i = codebuffer[cpos] - 192;
                    repbyte = codebuffer[cpos + 1];
                    do
                    {
                        if(x < maxwidth)
                        {
                            buf[dpos] = (signed char)repbyte;
                        }
                        ++dpos;
                        if((++x) >= pcx_header.bytesperline)
                        {
                            x = 0;
                            ++y;
                            dpos = y * maxwidth;
                        }
                    }
                    while(--i);
                    cpos += 2;
                }
                else
                {
                    if(x < maxwidth)
                    {
                        buf[dpos] = (signed char)codebuffer[cpos];
                    }
                    ++cpos;
                    ++dpos;
                    if((++x) >= pcx_header.bytesperline)
                    {
                        x = 0;
                        ++y;
                        dpos = y * maxwidth;
                    }
                }
            }
            spos += cpos;
        }
        free(codebuffer);
        codebuffer = NULL;
    }

    if(pal)
    {
        seekpackfile(handle, -768, SEEK_END);
        if(pb == 512) // 16bit 565
        {
            pbuf = malloc(768);
            if(readpackfile(handle, pbuf, 768) != 768)
            {
                free(pbuf);
                pbuf = NULL;
                return 0;
            }
            for(i = 0, j = 0; i < 512; i += 2, j += 3)
            {
                *(unsigned short *)(pal + i) = colour16(pbuf[j], pbuf[j + 1], pbuf[j + 2]);
            }
            free(pbuf);
            pbuf = NULL;
        }
        else if(pb == 768) // 24bit
        {
            if(readpackfile(handle, pal, 768) != 768)
            {
                return 0;
            }
        }
        else if(pb == 1024) // 32bit
        {
            pbuf = malloc(768);
            if(readpackfile(handle, pbuf, 768) != 768)
            {
                free(pbuf);
                pbuf = NULL;
                return 0;
            }
            for(i = 0, j = 0; i < 1024; i += 4, j += 3)
            {
                *(unsigned *)(pal + i) = colour32(pbuf[j], pbuf[j + 1], pbuf[j + 2]);
            }
            free(pbuf);
            pbuf = NULL;
        }
    }
    return 1;
}

//static void closepcx(){
//    closepackfile(handle);
//    handle = -1;
//}

// ============================== auto loading ===============================

#define OT_GIF 1
#define OT_BMP 2
#define OT_PCX 3
#define OT_PNG 4

static int open_type = 0;

static int openimage(char *filename, char *packfile)
{
    char fnam[128];
    int len = strlen(filename);
    char *ext = filename + len - 4;
    open_type = 0;

    if(0 == stricmp(ext, ".png") && openpng(filename, packfile))
    {
        open_type = OT_PNG;
        return 1;
    }
    else if(0 == stricmp(ext, ".gif") && opengif(filename, packfile))
    {
        open_type = OT_GIF;
        return 1;
    }
    else if(0 == stricmp(ext, ".pcx") && openpcx(filename, packfile))
    {
        open_type = OT_PCX;
        return 1;
    }
    else if(0 == stricmp(ext, ".bmp") && openbmp(filename, packfile))
    {
        open_type = OT_BMP;
        return 1;
    }

    sprintf(fnam, "%s.png", filename);
    if(openpng(fnam, packfile))
    {
        open_type = OT_PNG;
        return 1;
    }

    sprintf(fnam, "%s.gif", filename);
    if(opengif(fnam, packfile))
    {
        open_type = OT_GIF;
        return 1;
    }

    sprintf(fnam, "%s.pcx", filename);
    if(openpcx(fnam, packfile))
    {
        open_type = OT_PCX;
        return 1;
    }

    sprintf(fnam, "%s.bmp", filename);
    if(openbmp(fnam, packfile))
    {
        open_type = OT_BMP;
        return 1;
    }
    return 0;
}

static int readimage(unsigned char *buf, unsigned char *pal, int maxwidth, int maxheight)
{
    int result = 0;
    switch(open_type)
    {
    case OT_GIF:
        result = readgif(buf, pal, maxwidth, maxheight);
#ifdef VERBOSE
        printf("calling readimage %p %p %d %d with format %s, result is %d\n", buf, pal, maxwidth, maxheight, "GIF", result);
#endif
        break;
    case OT_PCX:
        result = readpcx(buf, pal, maxwidth, maxheight);
#ifdef VERBOSE
        printf("calling readimage %p %p %d %d with format %s, result is %d\n", buf, pal, maxwidth, maxheight, "PCX", result);
#endif
        break;
    case OT_BMP:
        result = readbmp(buf, pal, maxwidth, maxheight);
#ifdef VERBOSE
        printf("calling readimage %p %p %d %d with format %s, result is %d\n", buf, pal, maxwidth, maxheight, "BMP", result);
#endif
        break;
    case OT_PNG:
        result = readpng(buf, pal, maxwidth, maxheight);
#ifdef VERBOSE
        printf("calling readimage %p %p %d %d with format %s, result is %d\n", buf, pal, maxwidth, maxheight, "PNG", result);
#endif
        break;
    }
    if(pal)
    {
        memset(pal, 0, (PAL_BYTES) >> 8);
    }
    return result;
}

static void closeimage()
{
    if(open_type == OT_PNG)
    {
        closepng();
    }
    else
    {
        if(handle > 0)
        {
            closepackfile(handle);
        }
        handle = -1;
    }
}

// ============================== Interface ===============================

int loadscreen(char *filename, char *packfile, unsigned char *pal, int format, s_screen **screen)
{
    int result;
    unsigned char *p;
#ifdef VERBOSE
    printf("loadscreen called packfile: %s, filename %s\n", packfile, filename);
#endif
    if((*screen))
    {
        freescreen(screen);
    }
    if(!openimage(filename, packfile))
    {
        return 0;
    }
    if(!(*screen) || ((*screen)->width != res[0] && (*screen)->height != res[1] && (*screen)->pixelformat != format))
    {
        (*screen) = allocscreen(res[0], res[1], format);
        if((*screen) == NULL)
        {
            closeimage();
            assert(0);
            return 0;
        }
    }
    if(pal)
    {
        p = pal;
    }
    else
    {
        p = (*screen)->palette;
    }
    result = readimage((unsigned char *)(*screen)->data, p, (*screen)->width, (*screen)->height);
    closeimage();
    if(!result)
    {
        freescreen(screen);
        assert(0);
        return 0;
    }
    return 1;
}

s_bitmap *loadbitmap(char *filename, char *packfile, int format)
{
    int result;
    s_bitmap *bitmap;
    int maxwidth, maxheight;

    if(!openimage(filename, packfile))
    {
        return NULL;
    }

    maxwidth = res[0];
    maxheight = res[1];
    //if(maxwidth > 4096) maxwidth = 4096;
    //if(maxheight > 4096) maxheight = 4096;
    bitmap = allocbitmap(maxwidth, maxheight, format);
    if(!bitmap)
    {
        closeimage();
        return NULL;
    }

    result = readimage((unsigned char *)bitmap->data, bitmap->palette, maxwidth, maxheight);
    closeimage();
    if(!result)
    {
        freebitmap(bitmap);
        return NULL;
    }
    return bitmap;
}

int loadimagepalette(char *filename, char *packfile, unsigned char *pal)
{
    int result;

    if(!openimage(filename, packfile))
    {
        return 0;
    }

    result = readimage(NULL, pal, 0, 0);
    closeimage();
    return  result;
}
