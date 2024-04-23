/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

// Functions to load GIF, PCX and BMP files.
// Last update: 26-jan-2003
// Now loading to screens or bitmaps,
// creating them on-the-fly if necessary.

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>
#include "utils.h"
#include "types.h"
#include "borendian.h"
#include "bitmap.h"
#include "screen.h"
#include "packfile.h"
#include "pngdec.h"

#pragma pack (1)

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

// Open a BMP stream
static int openbmp(char *filename, char *packfilename)
{
    if((handle = openpackfile(filename, packfilename)) == -1)
    {
        return 0;
    }
    if(readpackfile(handle, &bmp_header, sizeof(s_bmpheader)) != sizeof(s_bmpheader))
    {
       closepackfile(handle);
        return 0;
    }
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

// ============================== PNG loading ===============================
// New PNG decoder by Plombo (2019-1-18) -- faster than the old libpng-based one

#define PNG_MAGIC        0x89504e470d0a1a0aLL
#define PNG_CHUNK_IHDR   0x49484452
#define PNG_CHUNK_PLTE   0x504c5445
#define PNG_CHUNK_IDAT   0x49444154

static int png_is_interlaced = 0;

struct png_chunk_header {
    uint32_t chunk_size;
    uint32_t chunk_name;
};

static void closepng()
{
    if (handle >= 0)
    {
        closepackfile(handle);
    }
    handle = -1;
}

// return 1 on success, 0 on error
static int openpng(const char *filename, const char *packfilename)
{
    static int warned_about_interlacing = 0;

    if ((handle = openpackfile(filename, packfilename)) == -1)
    {
        goto openpng_abort;
    }

    uint64_t magic;
    if (readpackfile(handle, &magic, 8) != 8)
    {
        goto openpng_abort;
    }

    if (magic != SwapMSB64(PNG_MAGIC))
    {
        goto openpng_abort;
    }

    struct png_chunk_header chunk_header;
    if (readpackfile(handle, &chunk_header, sizeof(chunk_header)) != sizeof(chunk_header))
    {
        goto openpng_abort;
    }
    if (SwapMSB32(chunk_header.chunk_size) != 13 || chunk_header.chunk_name != SwapMSB32(PNG_CHUNK_IHDR))
    {
        goto openpng_abort;
    }

    char ihdr_data[13];
    uint32_t *ihdr_data32 = (uint32_t *)ihdr_data;
    if (readpackfile(handle, &ihdr_data, sizeof(ihdr_data)) != sizeof(ihdr_data))
    {
        goto openpng_abort;
    }
    uint32_t width = SwapMSB32(ihdr_data32[0]);
    uint32_t height = SwapMSB32(ihdr_data32[1]);
    res[0] = width;
    res[1] = height;

    // Bit depth must be 8 for our purposes. Compression and filter method must be 0 in all PNGs.
    if (ihdr_data[8] != 8 || ihdr_data[10] != 0 || ihdr_data[11] != 0)
    {
        goto openpng_abort;
    }
    // Color type must be grayscale or indexed.
    else if (ihdr_data[9] != 0 && ihdr_data[9] != 3)
    {
        goto openpng_abort;
    }

    if (ihdr_data[12] == 1)
    {
        if (!warned_about_interlacing)
        {
            // Only print this warning once
            printf("Warning: The image %s is interlaced. For faster load times, use non-interlaced images.\n", filename);
            warned_about_interlacing = 1;
        }
    }
    // Interlacing mode must be either 0 (disabled) or 1 (Adam7).
    else if (ihdr_data[12] != 0)
    {
        goto openpng_abort;
    }

    png_is_interlaced = ihdr_data[12];

    return 1;

openpng_abort:
    closepackfile(handle);
    handle = -1;
    return 0;
}

// Based on the PaethPredictor pseudocode in the PNG specification.
// a = pixel to the left, b = above, c = upper left
static inline unsigned char png_paeth_predictor(unsigned char a, unsigned char b, unsigned char c)
{
    // initial estimate
    int p = a + b - c;

    // distances to a, b, c
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);
    
    // return nearest of a,b,c, breaking ties in the order a,b,c
    if (pa <= pb && pa <= pc) return a;
    else if (pb <= pc) return b;
    else return c;
}

// Decodes the image from a decompressed, non-interlaced IDAT stream.
static void png_decode_regular(unsigned char *buf, unsigned char *inflated_data, int max_width, int max_height)
{
    int width = res[0];
    unsigned int y, x;

    for (y = 0; y < max_height; y++)
    {
        switch (inflated_data[y * (width + 1)])
        {
            case 0: // no filter, the easiest case
            {
                memcpy(buf + (y * max_width), inflated_data + (y * (width + 1)) + 1, max_width);
                break;
            }
            case 1: // Sub filter: Raw(x) = Sub(x) + Raw(pixel to the left of x)
            {
                unsigned char last = 0;
                for (x = 0; x < max_width; x++)
                {
                    last = buf[y * max_width + x] = inflated_data[y * (width + 1) + 1 + x] + last;
                }
                break;
            }
            case 2: // Up filter: Raw(x) = Up(x) + Raw(pixel above x)
            {
                if (y == 0)
                {
                    memcpy(buf + (y * max_width), inflated_data + (y * (width + 1)) + 1, max_width);
                }
                else
                {
                    unsigned int lastline = y - 1;
                    for (x = 0; x < max_width; x++)
                    {
                        buf[y * max_width + x] = inflated_data[y * (width + 1) + 1 + x] + buf[lastline * max_width + x];
                    }
                }
                break;
            }
            case 3: // Average filter: Raw(x) = Average(x) + floor((Raw(pixel above x) + Raw(pixel left of x))/2)
            {
                unsigned char last = 0;
                unsigned int lastline = y - 1;
                for (x = 0; x < max_width; x++)
                {
                    unsigned char a = last;
                    unsigned char b = (y == 0) ? 0 : buf[lastline * max_width + x];
                    last = buf[y * max_width + x] = inflated_data[y * (width + 1) + 1 + x] + ((a + b) / 2);
                }
                break;
            }
            case 4: // Paeth filter: the complicated one
            {
                unsigned char last = 0;
                unsigned int lastline = y - 1;
                for (x = 0; x < max_width; x++)
                {
                    unsigned char a = last;
                    unsigned char b = (y == 0) ? 0 : buf[lastline * max_width + x];
                    unsigned char c = (y == 0 || x == 0) ? 0 : buf[lastline * max_width + x - 1];
                    last = buf[y * max_width + x] = inflated_data[y * (width + 1) + 1 + x] + png_paeth_predictor(a, b, c);
                }
                break;
            }
            default:
            {
                printf("invalid PNG filter %i for line %u\n", inflated_data[y * (width + 1)], y);
                assert(!"invalid PNG filter");
            }
        }
    }
}

// Decodes the image from a decompressed, interlaced IDAT stream.
static void png_decode_interlaced(unsigned char *buf, unsigned char *inflated_data, int max_width, int max_height)
{
    int width = res[0], height = res[1];
    const int start_y[7] =  {0, 0, 4, 0, 2, 0, 1};
    const int start_x[7] =  {0, 4, 0, 2, 0, 1, 0};
    const int y_increment[7] = {8, 8, 8, 4, 4, 2, 2};
    const int x_increment[7] = {8, 8, 4, 4, 2, 2, 1};
    int pass;

    for (pass = 0; pass < 7; pass++)
    {
        unsigned int yin, yout, xin, xout;

        int line_width = (width + x_increment[pass] - start_x[pass] - 1) / x_increment[pass];
        if (line_width == 0)
        {
            continue;
        }

        for (yin = 0, yout = start_y[pass]; yout < max_height; yin++, yout += y_increment[pass])
        {
            switch (inflated_data[yin * (line_width + 1)])
            {
                case 0: // no filter, the easiest case
                {
                    for (xin = 0, xout = start_x[pass]; xout < max_width; xin++, xout += x_increment[pass])
                    {
                        buf[yout * max_width + xout] = inflated_data[yin * (line_width + 1) + 1 + xin];
                    }
                    break;
                }
                case 1: // Sub filter: Raw(x) = Sub(x) + Raw(pixel to the left of x)
                {
                    unsigned char last = 0;
                    for (xin = 0, xout = start_x[pass]; xout < max_width; xin++, xout += x_increment[pass])
                    {
                        last = buf[yout * max_width + xout] = inflated_data[yin * (line_width + 1) + 1 + xin] + last;
                    }
                    break;
                }
                case 2: // Up filter: Raw(x) = Up(x) + Raw(pixel above x)
                {
                    if (yin == 0)
                    {
                        for (xin = 0, xout = start_x[pass]; xout < max_width; xin++, xout += x_increment[pass])
                        {
                            buf[yout * max_width + xout] = inflated_data[yin * (line_width + 1) + 1 + xin];
                        }
                    }
                    else
                    {
                        unsigned int lastline = yout - y_increment[pass];
                        for (xin = 0, xout = start_x[pass]; xout < max_width; xin++, xout += x_increment[pass])
                        {
                            buf[yout * max_width + xout] = inflated_data[yin * (line_width + 1) + 1 + xin] + buf[lastline * max_width + xout];
                        }
                    }
                    break;
                }
                case 3: // Average filter: Raw(x) = Average(x) + floor((Raw(pixel above x) + Raw(pixel left of x))/2)
                {
                    unsigned char last = 0;
                    unsigned int lastline = yout - y_increment[pass];
                    for (xin = 0, xout = start_x[pass]; xout < max_width; xin++, xout += x_increment[pass])
                    {
                        unsigned char a = last;
                        unsigned char b = (yin == 0) ? 0 : buf[lastline * max_width + xout];
                        last = buf[yout * max_width + xout] = inflated_data[yin * (line_width + 1) + 1 + xin] + ((a + b) / 2);
                    }
                    break;
                }
                case 4: // Paeth filter: the complicated one
                {
                    unsigned char last = 0;
                    unsigned int lastline = yout - y_increment[pass];
                    for (xin = 0, xout = start_x[pass]; xout < max_width; xin++, xout += x_increment[pass])
                    {
                        unsigned char a = last;
                        unsigned char b = (yin == 0) ? 0 : buf[lastline * max_width + xout];
                        unsigned char c = (yin == 0 || xin == 0) ? 0 : buf[lastline * max_width + xout - x_increment[pass]];
                        last = buf[yout * max_width + xout] = inflated_data[yin * (line_width + 1) + 1 + xin] + png_paeth_predictor(a, b, c);
                    }
                    break;
                }
                default:
                {
                    assert(!"invalid PNG filter");
                }
            }
        }

        inflated_data += (line_width + 1) * ((height + y_increment[pass] - start_y[pass] - 1) / y_increment[pass]);
    }
}

static int readpng(unsigned char *buf, unsigned char *pal, int max_width, int max_height)
{
    unsigned char *png_data = NULL, *png_data_ptr;
    unsigned char *inflated_data = NULL;
    z_stream zlib_stream = {.zalloc = Z_NULL, .zfree = Z_NULL, .opaque = Z_NULL, .avail_in = 0, .next_in = Z_NULL,
                            .avail_out = 0, .next_out = Z_NULL};
    int width = res[0], height = res[1];

    if (inflateInit(&zlib_stream) != Z_OK)
    {
        goto readpng_abort;
    }

    // Read the rest of the file into a single chunk of memory to save on expensive I/O operations.
    int data_start_pos = seekpackfile(handle, 0, SEEK_CUR) + 4; // +4 bytes to skip the CRC at the end of IHDR chunk
    int data_size = seekpackfile(handle, 0, SEEK_END) - data_start_pos;
    seekpackfile(handle, data_start_pos, SEEK_SET);
    png_data = malloc(data_size);

    if (!png_data)
    {
        goto readpng_abort;
    }
    else if (readpackfile(handle, png_data, data_size) != data_size)
    {
        goto readpng_abort;
    }
    png_data_ptr = png_data;

    if (buf)
    {
        // the "+1"s are because each scanline has an extra byte denoting the filter type
        size_t inflated_size;
        if (png_is_interlaced)
        {
            inflated_size = ((width + 7) / 8 + 1) * ((height + 7) / 8) +
                            ((width + 3) / 8 + 1) * ((height + 7) / 8) +
                            ((width + 3) / 4 + 1) * ((height + 3) / 8) +
                            ((width + 1) / 4 + 1) * ((height + 3) / 4) +
                            ((width + 1) / 2 + 1) * ((height + 1) / 4) +
                            (width / 2 + 1) * ((height + 1) / 2) +
                            (width + 1) * (height / 2);
        }
        else
        {
            inflated_size = (width + 1) * height;
        }

        inflated_data = malloc(inflated_size);
        zlib_stream.avail_out = inflated_size;
        zlib_stream.next_out = inflated_data;
    }

    // Now read the remaining chunks of the file
    while (png_data_ptr < (png_data + data_size))
    {
        struct png_chunk_header *p_chunk_header = (struct png_chunk_header *) png_data_ptr;
        uint32_t chunk_size = SwapMSB32(p_chunk_header->chunk_size);
        png_data_ptr += sizeof(*p_chunk_header);

        // PLTE chunk: contains the palette
        if (pal && p_chunk_header->chunk_name == SwapMSB32(PNG_CHUNK_PLTE))
        {
            unsigned int ncolors = chunk_size / 3, i;
            int *pal32 = (int*) pal;
            if (chunk_size % 3 != 0)
            {
                goto readpng_abort;
            }
            for (i = 0; i < ncolors; i++)
            {
                pal32[i] = colour32(png_data_ptr[0], png_data_ptr[1], png_data_ptr[2]);
                png_data_ptr += 3;
            }
            png_data_ptr += 4;
        }
        /* IDAT chunks contain the actual image data, compressed with DEFLATE in the zlib format. There can be
           multiple IDAT chunks, but their data together forms a single compressed stream. */
        else if (buf && p_chunk_header->chunk_name == SwapMSB32(PNG_CHUNK_IDAT))
        {
            zlib_stream.avail_in = chunk_size;
            zlib_stream.next_in = png_data_ptr;
            int zret;
            zret = inflate(&zlib_stream, Z_SYNC_FLUSH);
            if (zret == Z_STREAM_END)
            {
                break;
            }
            else if (zret != Z_OK)
            {
                printf("inflate failed: %i\n", zret);
                goto readpng_abort;
            }
            png_data_ptr += chunk_size + 4;
        }
        else
        {
            png_data_ptr += chunk_size + 4;
        }
    }

    if (buf)
    {
        if (zlib_stream.avail_out != 0)
        {
            // For very small interlaced images, we may overestimate the inflated size by a few bytes, because the
            // size calculation includes filter bytes for lines of width 0. That's harmless, but if the inflated
            // data for any other kind of image doesn't fill the buffer, then the image data is incomplete.
            if (!(png_is_interlaced && width < 8))
            {
                printf("error: incomplete compressed stream\n");
                goto readpng_abort;
            }
        }

        if (png_is_interlaced)
        {
            png_decode_interlaced(buf, inflated_data, max_width, max_height);
        }
        else
        {
            png_decode_regular(buf, inflated_data, max_width, max_height);
        }
    }

    inflateEnd(&zlib_stream);
    free(inflated_data);
    free(png_data);
    return 1;

readpng_abort:
    inflateEnd(&zlib_stream);
    free(inflated_data);
    free(png_data);
    return 0;
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

typedef struct
{
    unsigned short   left, top, width, height;
    unsigned char    flags;
} gifblockstruct;

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

    if(readpackfile(handle, &gif_header, sizeof(gifheaderstruct)) != sizeof(gifheaderstruct))
    {
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

            if(readpackfile(handle, &iblock, sizeof(iblock)) != sizeof(iblock))
            {
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
            //assert(0);
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
        //assert(0);
        return 0;
    }
    return 1;
}

int loadscreen32(char *filename, char *packfile, s_screen **screen)
{
    void *data;
    int handle, filesize;
    char fnam[128];
#ifdef VERBOSE
    printf("loadscreen called packfile: %s, filename %s\n", packfile, filename);
#endif
    if((*screen))
    {
        freescreen(screen);
    }

    if((handle = openpackfile(filename, packfile)) == -1)
    {
        sprintf(fnam, "%s.png", filename);
        if((handle = openpackfile(fnam, packfile)) == -1)
        {
            return 0;
        }
    }
    filesize = seekpackfile(handle, 0, SEEK_END);
    data = malloc(filesize);
    assert(seekpackfile(handle, 0, SEEK_SET) == 0);
    if (!data || readpackfile(handle, data, filesize) != filesize)
    {
        closepackfile(handle);
        free(data);
        return 0;
    }
    closepackfile(handle);

    (*screen) = pngToScreen(data);
    free(data);
    if (!(*screen)) return 0;
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
