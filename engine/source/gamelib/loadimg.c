/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

/*
* Functions to load PNG images into 
* screens or bitmaps. Legacy BMP, PCX, 
* and still GIF support removed 2026-06-01.
*
* Last update: 2026-06-01 by Damon V. Caskey.
* Now loading to screens or bitmaps,
* creating them on-the-fly if necessary.
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <zlib.h>

#include "utils.h"
#include "types.h"
#include "borendian.h"
#include "bitmap.h"
#include "screen.h"
#include "packfile.h"
#include "pngdec.h"

// ============================== Globals ===============================
#define HANDLE_UNUSED -1
static int image_load_handle = HANDLE_UNUSED;

/*
* Caskey, Damon V.
* Original date and author unknown, reworked 2026-06-01.
*
* Resolution of the currently opened image. Image open 
* functions populate this value after validating the 
* file header. loadbitmap() uses it to allocate the 
* destination bitmap before readimage() decodes pixel 
* data.
*/

typedef struct image_resolution {
    int width;
    int height;
} image_resolution;

static image_resolution image_res = {.width = 0, .height = 0};

// ============================== PNG loading ===============================
// New PNG decoder by Plombo (2019-1-18) -- faster than the old libpng-based one
// 
// Caskey, Damon V., 2026-06-01 - Minor readability updates, clean up for 64-bit 
// Win and drop interlaced support.

#define PNG_MAGIC        0x89504e470d0a1a0aLL
#define PNG_CHUNK_IHDR   0x49484452
#define PNG_CHUNK_PLTE   0x504c5445
#define PNG_CHUNK_IDAT   0x49444154
#define PNG_CHUNK_IEND   0x49454e44

struct png_chunk_header {
    uint32_t chunk_size;
    uint32_t chunk_name;
};

static void closepng()
{
    if (image_load_handle >= 0)
    {
        closepackfile(image_load_handle);
    }
    image_load_handle = HANDLE_UNUSED;
}

/*
* Caskey, Damon V.
* Original date and author unknown, reworked 2026-06-01.
*
* Opens a PNG image from disk or the active pack file and reads
* enough header information to validate the image and populate the
* global image resolution values.
*
* Only 8-bit grayscale and indexed PNG images are supported. RGB,
* RGBA, grayscale-alpha, and non-standard compression or filter
* methods are rejected.
*
* Returns 1 on success, or 0 on error. On success, the pack file
* handle remains open for readpng() and must later be released by
* closepng().
*/
static int openpng(const char *filename, const char *packfilename, int report_color_type_error) {

    uint64_t magic;
    struct png_chunk_header chunk_header;
    unsigned char ihdr_data[13];
    uint32_t width_raw;
    uint32_t height_raw;
    uint32_t width;
    uint32_t height;

#ifdef VERBOSE
    printf("openpng: entering filename='%s' pack='%s'\n",
           filename,
           packfilename ? packfilename : "(null)");
#endif

    /*
    * Close any previous image handle before attempting a 
    * new open. This prevents stale handles from surviving 
    * across failed opens.
    */
    closepng();

    image_load_handle = openpackfile(filename, packfilename);
    if(image_load_handle == HANDLE_UNUSED) {
        goto openpng_abort;
    }

#ifdef VERBOSE
    printf("openpng: openpackfile OK handle=%d\n", image_load_handle);
#endif

    /*
    * Validate the 8 byte PNG signature. SwapMSB64() must use a true
    * 64-bit integer type on all platforms, including Win64.
    */
    if(readpackfile(image_load_handle, &magic, 8) != 8) {
        goto openpng_abort;
    }

    if(magic != SwapMSB64(PNG_MAGIC)) {
        goto openpng_abort;
    }

    /*
    * The first PNG chunk must be IHDR and its payload must be 13 bytes.
    */
    if(readpackfile(image_load_handle, &chunk_header, sizeof(chunk_header)) != sizeof(chunk_header)) {
        goto openpng_abort;
    }

    if(SwapMSB32(chunk_header.chunk_size) != 13 ||
            chunk_header.chunk_name != SwapMSB32(PNG_CHUNK_IHDR)) {
        goto openpng_abort;
    }

    /*
    * Read the IHDR payload. Use memcpy() instead of casting the byte
    * buffer to uint32_t*. Casting can violate alignment and aliasing
    * rules, especially on 64-bit builds.
    */
    if(readpackfile(image_load_handle, ihdr_data, sizeof(ihdr_data)) != sizeof(ihdr_data)) {
        goto openpng_abort;
    }

    memcpy(&width_raw, ihdr_data + 0, sizeof(width_raw));
    memcpy(&height_raw, ihdr_data + 4, sizeof(height_raw));

    width = SwapMSB32(width_raw);
    height = SwapMSB32(height_raw);

    /* 
    * Check for zero dimensions, and also guard against 
    * 32-bit integer overflow when converting to int. 
    */
   
    if(width == 0 || height == 0 || width > INT_MAX || height > INT_MAX) {
        printf("\n\n Error: The image %s has invalid dimensions.\n", filename);
        goto openpng_abort;
    }

    image_res.width = (int)width;
    image_res.height = (int)height;

    /*
    * Bit depth must be 8 for engine use. PNG compression and filter
    * method must be 0 by specification.
    */
    if(ihdr_data[8] != 8 || ihdr_data[10] != 0 || ihdr_data[11] != 0) {
        printf("\n\n Error: The image %s has unsupported bit depth or compression/filter method. Use 8-bit PNG images with standard compression and filter methods.\n", filename);
        goto openpng_abort;
    }

    /*
    * Color type must be grayscale or indexed. Truecolor, alpha, and
    * truecolor-alpha PNG images are not supported by this decoder.
    *
    * Normal indexed image loads report the mismatch loudly. Background
    * loading may suppress this one diagnostic while probing the indexed
    * path because it immediately falls back to the RGB/RGBA decoder.
    */
    if(ihdr_data[9] != 0 && ihdr_data[9] != 3) {
        if(report_color_type_error) {
            printf("\n\n Error: The image %s has unsupported color type. Use indexed PNG images.\n", filename);
        }
#ifdef VERBOSE
        else {
            printf("openpng: indexed background probe skipped color type %u for '%s'\n",
                   ihdr_data[9],
                   filename);
        }
#endif
        goto openpng_abort;
    }

    /*
    * Interlaced PNGs are not supported. Local packed assets get no
    * practical benefit from Adam7 interlacing, and supporting it adds
    * decoder complexity. Use non-interlaced PNGs.
    */
    if(ihdr_data[12] != 0) {
        printf("\n\n Error: The image %s is interlaced. Use non-interlaced PNG images.\n", filename);
        goto openpng_abort;
    }

    return 1;

openpng_abort:

    /*
    * If the PNG failed after opening a file or pack entry, release the
    * handle before returning failure. If no handle was opened, leave the
    * close path alone.
    */
    if(image_load_handle != HANDLE_UNUSED) {
        closepackfile(image_load_handle);
        image_load_handle = HANDLE_UNUSED;
    }

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
static void png_decode_data(unsigned char *buf, unsigned char *inflated_data, int max_width, int max_height)
{
    int width = image_res.width;
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

/*
* Caskey, Damon V.
* Original date and author unknown, reworked 2026-06-01.
*
* Reads the PNG image data from the active pack 
* file handle, inflates the IDAT chunks, and applies 
* PNG filters. 2026-06-01 rework adds documentation 
* and shores up some edge cases for PNG decoding and
* 64-bit compatibility.
*/
static int readpng(unsigned char *buf, unsigned char *pal, int max_width, int max_height) {

    unsigned char *png_data = NULL;
    unsigned char *png_data_ptr = NULL;
    unsigned char *inflated_data = NULL;
    z_stream zlib_stream = {
        .zalloc = Z_NULL, 
        .zfree = Z_NULL, 
        .opaque = Z_NULL, 
        .avail_in = 0, 
        .next_in = Z_NULL,
        .avail_out = 0, 
        .next_out = Z_NULL
    };

    int width = image_res.width;
    int height = image_res.height;
    int data_start_pos;
    int data_size;
    int z_lib_result = Z_OK;
    int zlib_initialized = 0;

    /*
    * Initialize the zlib stream before reading IDAT chunks.
    * readpng_abort calls inflateEnd(), so any failure after this
    * point can use the common cleanup path. We then set a flag 
    * to indicate that the zlib stream has been initialized for
    * downstream guards.
    */
    if(inflateInit(&zlib_stream) != Z_OK) {
        goto readpng_abort;
    }

    zlib_initialized = 1;

    /*
    * Read the remaining PNG data into memory. openpng() already read
    * the PNG signature, IHDR chunk header, and IHDR payload. The file
    * pointer is currently positioned just before the IHDR CRC, so skip
    * four bytes to move to the next chunk.
    *
    * Loading the rest of the file into one buffer avoids repeated pack
    * file I/O while walking PNG chunks.
    */
    int current_pos;
    int end_pos;

    current_pos = seekpackfile(image_load_handle, 0, SEEK_CUR);
    end_pos = seekpackfile(image_load_handle, 0, SEEK_END);

    if(current_pos < 0 || end_pos < 0) {
        goto readpng_abort;
    }

    data_start_pos = current_pos + 4;

    if(data_start_pos < current_pos || end_pos <= data_start_pos) {
        goto readpng_abort;
    }

    data_size = end_pos - data_start_pos;

    if(data_size <= 0) {
        goto readpng_abort;
    }

    if(seekpackfile(image_load_handle, data_start_pos, SEEK_SET) < 0) {
        goto readpng_abort;
    }

    png_data = malloc(data_size);

    if(!png_data) {
        goto readpng_abort;
    }

    if(readpackfile(image_load_handle, png_data, data_size) != data_size) {
        goto readpng_abort;
    }

    png_data_ptr = png_data;    

    /*
    * If the caller supplied a pixel buffer, allocate space for the
    * decompressed scanlines. Each PNG scanline begins with one filter
    * byte, so the decompressed size is one extra byte per row.
    *
    * If buf is NULL, only palette data is being requested and the IDAT
    * stream does not need to be inflated.
    */

    if(buf) {

        size_t inflated_size;

        inflated_size = ((size_t)width + 1) * (size_t)height;

        if(inflated_size > UINT_MAX) {
            goto readpng_abort;
        }

        inflated_data = malloc(inflated_size);

        if(!inflated_data) {
            goto readpng_abort;
        }

        zlib_stream.avail_out = (uInt)inflated_size;
        zlib_stream.next_out = inflated_data;
    }

    /*
    * Walk the remaining PNG chunks. The loader only needs PLTE for the
    * palette and IDAT for pixel data. Other chunks are skipped.
    *
    * Chunk layout:
    *   4 bytes - data length
    *   4 bytes - chunk name
    *   N bytes - chunk data
    *   4 bytes - CRC
    */

    while(png_data_ptr < (png_data + data_size)) {

        struct png_chunk_header chunk_header;
        uint32_t chunk_size;
        uint32_t chunk_name;
        ptrdiff_t bytes_left;

        bytes_left = (png_data + data_size) - png_data_ptr;

        /*
        * Make sure there is enough data left to read the chunk header.
        * Using memcpy() avoids alignment and aliasing issues on 64-bit
        * builds.
        */
        if(bytes_left < (ptrdiff_t)sizeof(chunk_header)) {
            goto readpng_abort;
        }

        memcpy(&chunk_header, png_data_ptr, sizeof(chunk_header));

        chunk_size = SwapMSB32(chunk_header.chunk_size);
        chunk_name = chunk_header.chunk_name;

        png_data_ptr += sizeof(chunk_header);
        bytes_left = (png_data + data_size) - png_data_ptr;

        /*
        * Make sure the declared chunk payload and CRC are still inside
        * the data buffer before reading or skipping the chunk.
        */
        if(bytes_left < 4 || chunk_size > (uint32_t)(bytes_left - 4)) {
            goto readpng_abort;
        }

        /*
        * IEND marks the formal end of the PNG stream. Stop here so
        * palette-only reads do not keep walking into trailing data.
        */
        if(chunk_name == SwapMSB32(PNG_CHUNK_IEND)) {
            png_data_ptr += chunk_size + 4;
            break;
        }

        /*
        * PLTE contains the image palette. This loader currently writes
        * palette entries in the engine's 32-bit palette format.
        */
        if(pal && chunk_name == SwapMSB32(PNG_CHUNK_PLTE)) {

            unsigned int ncolors = chunk_size / 3;
            unsigned int i;
            int *pal32 = (int*)pal;

            if(chunk_size % 3 != 0 || ncolors > 256) {
                goto readpng_abort;
            }

            for(i = 0; i < ncolors; i++) {
                pal32[i] = colour32(png_data_ptr[0], png_data_ptr[1], png_data_ptr[2]);
                png_data_ptr += 3;
            }

            /*
            * Skip the PLTE CRC after reading the palette payload.
            */
            png_data_ptr += 4;

        } else if(buf && chunk_name == SwapMSB32(PNG_CHUNK_IDAT)) {

            /*
            * IDAT chunks contain one continuous zlib stream split across
            * one or more PNG chunks. Feed each IDAT payload to inflate()
            * in file order.
            */
            zlib_stream.avail_in = chunk_size;
            zlib_stream.next_in = png_data_ptr;

            z_lib_result = inflate(&zlib_stream, Z_SYNC_FLUSH);

            /*
            * Move past the IDAT payload and CRC before checking whether
            * zlib reached the end of the stream.
            */
            png_data_ptr += chunk_size + 4;

            if(z_lib_result == Z_STREAM_END) {
                break;
            }

            if(z_lib_result != Z_OK) {
                printf("inflate failed: %i\n", z_lib_result);
                goto readpng_abort;
            }

        } else {

            /*
            * Unused chunk. Skip payload and CRC.
            */
            png_data_ptr += chunk_size + 4;
        }
    }

    /*
    * If the caller requested pixel data, the zlib stream must have
    * reached its natural end and the output buffer must be filled
    * with exact pixel data. If not, something went wrong.
    */

    if(buf && (z_lib_result != Z_STREAM_END || zlib_stream.avail_out != 0)) {
        goto readpng_abort;
    }

    /*
    * Decode the decompressed scanlines into the destination buffer.
    * PNG filtering is reversed here after all IDAT data has been
    * inflated.
    */

    if(buf) {
        png_decode_data(buf, inflated_data, max_width, max_height);
    }

    if(zlib_initialized) {
        inflateEnd(&zlib_stream);
    }

    free(inflated_data);
    free(png_data);
    return 1;

readpng_abort:

    /*
    * Shared cleanup path for allocation, file read, PNG chunk, and zlib
    * failures.
    */
    
    if(zlib_initialized) {
        inflateEnd(&zlib_stream);
    }

    free(inflated_data);
    free(png_data);
    return 0;
}

// ============================== auto loading ===============================
/*
* Caskey, Damon V.
* Simplified still image loader to PNG only 2026-06-01.
*/

/*
* Image format currently opened by openimage().
* readimage() and closeimage() use this value to
* route follow up work to the correct image loader.
*/
typedef enum open_type_enum {
    OT_NONE = 0,
    OT_PNG
} open_type_enum;
    
static open_type_enum open_type = OT_NONE;

/*
* Caskey, Damon V.
* Added 2026-06-01.
*
* Finds the file extension in an image path. Dots 
* in directory names are ignored, so paths like 
* data/v1.0/sprite still count as extensionless.
*/
static const char *get_image_extension(const char *filename) {
    const char *ext = strrchr(filename, '.');
    const char *slash = strrchr(filename, '/');
    const char *backslash = strrchr(filename, '\\');

    if(!ext) {
        return NULL;
    }

    if(slash && ext < slash) {
        return NULL;
    }

    if(backslash && ext < backslash) {
        return NULL;
    }

    return ext;
}

/*
* Caskey, Damon V.
* Original date and author unknown, reworked 2026-06-01.
*
* Opens a PNG image from disk or the active pack file and records the
* detected image type for later readimage() and closeimage() calls.
*
* If filename already includes .png, only that exact file is tested.
* This avoids opening a valid image more than once and leaking pack
* file handles.
*
* If filename has no supported extension, .png is appended and tested.
*
* Returns 1 on success, or 0 if no supported image could be opened.
*/
static int openimage(char *filename, char *packfile, int report_color_type_error) {

    char fnam[MAX_BUFFER_LEN];
    const char *ext = NULL;

    /*
    * Reset active image state before attempting a new open.
    * This prevents readimage(), closeimage(), or callers from using
    * stale state after a failed open attempt.
    */
    open_type = OT_NONE;
    image_res.width = 0;
    image_res.height = 0;

#ifdef VERBOSE
    printf("openimage: filename='%s' pack='%s'\n",
        filename,
        packfile ? packfile : "(null)");
#endif

    ext = get_image_extension(filename);

    /*
    * If the caller supplied an explicit extension, 
    * verify that it is a supported format and try 
    * to open it directly.
    */

    if(ext) {

        if(stricmp(ext, ".png") == 0) {

            if(openpng(filename, packfile, report_color_type_error)) {
                open_type = OT_PNG;
                return 1;
            }

            return 0;
        }

        printf("\n\n Error: Unsupported image format '%s' for file '%s'. Use non-interlaced indexed PNG images.\n", ext, filename);
        return 0;
    }

    /*
    * No extension was supplied. Try PNG by appending .png.
    * Return immediately on success so exactly one image 
    * handle remains active for readimage().
    */

    snprintf(fnam, sizeof(fnam), "%s.png", filename);
    if(openpng(fnam, packfile, report_color_type_error)) {
        open_type = OT_PNG;
        return 1;
    }

    return 0;
}

static int readimage(unsigned char *buf, unsigned char *pal, int maxwidth, int maxheight)
{
    int result = 0;

    switch(open_type)
    {
    case OT_PNG:
        result = readpng(buf, pal, maxwidth, maxheight);
#ifdef VERBOSE
        printf("calling readimage %p %p %d %d with format %s, result is %d\n",
            buf, pal, maxwidth, maxheight, "PNG", result);
#endif
        break;

    case OT_NONE:
    default:
        assert(!"invalid open_type in readimage");
        break;
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
        /*
        * Safety cleanup for failed or unexpected states. handle 0 is a
        * valid pack handle, so check for >= 0 rather than > 0.
        */
        if(image_load_handle >= 0)
        {
            closepackfile(image_load_handle);
        }

        image_load_handle = HANDLE_UNUSED;
    }

    open_type = OT_NONE;
}

// ============================== Interface ===============================

/*
* Caskey, Damon V.
* 2026-08-27
*
* Shared indexed screen loader. report_color_type_error controls only
* the diagnostic for an incompatible PNG color type. Every other image
* validation error remains loud.
*/
static int loadscreen_internal(char *filename, char *packfile, unsigned char *pal, int format, s_screen **screen, int report_color_type_error) {
    int result;
    unsigned char *p;

    #ifdef VERBOSE
        printf("loadscreen called packfile: %s, filename %s\n", packfile, filename);
    #endif

    if((*screen)) {
        freescreen(screen);
    }

    if(!openimage(filename, packfile, report_color_type_error)){
        return 0;
    }

    if(!(*screen) 
        || (*screen)->width != image_res.width 
        || (*screen)->height != image_res.height 
        || (*screen)->pixelformat != format) {

        (*screen) = allocscreen(image_res.width, image_res.height, format);
        
        if((*screen) == NULL) {
            closeimage();
            //assert(0);
            return 0;
        }
    }

    if(pal) {
        p = pal;
    
    } else {
        p = (*screen)->palette;
    }

    result = readimage((unsigned char *)(*screen)->data, p, (*screen)->width, (*screen)->height);
    closeimage();
    
    if(!result) {

        freescreen(screen);
        //assert(0);
        return 0;
    }

    return 1;
}

int loadscreen(char *filename, char *packfile, unsigned char *pal, int format, s_screen **screen) {
    return loadscreen_internal(filename, packfile, pal, format, screen, 1);
}

/*
* Caskey, Damon V.
* 2026-08-27
*
* Probe an image through the indexed screen decoder without reporting
* an incompatible color type. Background loading uses this probe before
* falling back to loadscreen32(). Other validation errors remain loud.
*/
int loadscreen_indexed_probe(char *filename, char *packfile, unsigned char *pal, int format, s_screen **screen) {
    return loadscreen_internal(filename, packfile, pal, format, screen, 0);
}

/*
* Caskey, Damon V.
* Original date and author unknown, reworked 2026-06-01.
*
* Loads a PNG image into a screen. The screen is 
* allocated on-the-fly. Rework removes legacy BMP, PCX, 
* and GIF support, simplifies the interface, and replaces 
* asserts with defensive checks that return failure. The 
* caller is responsible for handling the failure.
*/
int loadscreen32(char *filename, char *packfile, s_screen **screen) {

    void *data;
    int handle, filesize;
    char fnam[MAX_BUFFER_LEN];

    #ifdef VERBOSE
        printf("loadscreen called packfile: %s, filename %s\n", packfile, filename);
    #endif

    if((*screen)) {
        freescreen(screen);
    }

    if((handle = openpackfile(filename, packfile)) == HANDLE_UNUSED) {

        snprintf(fnam, sizeof(fnam), "%s.png", filename);
        
        if((handle = openpackfile(fnam, packfile)) == HANDLE_UNUSED) {
            return 0;
        }
    }

    filesize = seekpackfile(handle, 0, SEEK_END);

    if(filesize <= 0) {
        closepackfile(handle);
        return 0;
    }

    data = malloc(filesize);
    
    if(!data) {
        closepackfile(handle);
        return 0;
    }

    if(seekpackfile(handle, 0, SEEK_SET) != 0) {
        closepackfile(handle);
        free(data);
        return 0;
    }

    if(readpackfile(handle, data, filesize) != filesize) {
        closepackfile(handle);
        free(data);
        return 0;
    }

    closepackfile(handle);

    (*screen) = pngToScreen(data);
    free(data);
    
    if (!(*screen)) { 
        return 0;
    }

    return 1;
}

s_bitmap *loadbitmap(char *filename, char *packfile, int format)
{
    int result;
    s_bitmap *bitmap;
    int maxwidth, maxheight;

    #ifdef VERBOSE
        printf("loadbitmap: file='%s' pack='%s' format=%d\n",
           filename,
           packfile ? packfile : "(null)",
           format);
    #endif

    if(!openimage(filename, packfile, 1))
    {
        #ifdef VERBOSE
            printf("loadbitmap: openimage FAILED for '%s'\n", filename);
        #endif

        closeimage();
        return NULL;
    }

    #ifdef VERBOSE
        printf("loadbitmap: openimage OK, res=%d x %d\n", image_res.width, image_res.height);
    #endif

    maxwidth = image_res.width;
    maxheight = image_res.height;

    bitmap = allocbitmap(maxwidth, maxheight, format);
    if(!bitmap)
    {
        #ifdef VERBOSE
            printf("loadbitmap: allocbitmap FAILED, %d x %d format=%d\n",
                   maxwidth, maxheight, format);
        #endif

        closeimage();
        return NULL;
    }

    #ifdef VERBOSE
        printf("loadbitmap: bitmap=%p data=%p palette=%p\n",
               (void *)bitmap,
               (void *)bitmap->data,
               (void *)bitmap->palette);
    #endif

    result = readimage((unsigned char *)bitmap->data,
                       bitmap->palette,
                       maxwidth,
                       maxheight);

    closeimage();

    if(!result) {

        #ifdef VERBOSE
            printf("loadbitmap: readimage FAILED for '%s'\n", filename);
        #endif

        freebitmap(bitmap);
        return NULL;
    }

    #ifdef VERBOSE
        printf("loadbitmap: readimage OK for '%s'\n", filename);
    #endif

    return bitmap;
}

int loadimagepalette(char *filename, char *packfile, unsigned char *pal)
{
    int result;

    if(!openimage(filename, packfile, 1))
    {
        return 0;
    }

    result = readimage(NULL, pal, 0, 0);
    closeimage();
    return  result;
}
