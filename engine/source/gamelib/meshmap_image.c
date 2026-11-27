/*
 * OpenBOR - https://www.chronocrash.com
 * Copyright (c) OpenBOR Team. All rights reserved. See LICENSE.
 *
 * Damon V. Caskey
 * 2026-11-27
 * Terrain PNG input keeps source data separate from display preparation.
 * The engine already links libpng on every supported port; using its bounded
 * packfile reader gives geometry the same archive support as other assets.
 */
#include "meshmap_image.h"
#include "packfile.h"
#include "screen.h"
#include <png.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Packfile/screen headers expose the engine's fatal allocation wrappers.
 * Replacement loading must report failure while retaining the old map, so
 * these bounded temporary allocations use the standard nullable functions. */
#undef malloc
#undef calloc
#undef realloc

typedef struct meshmap_png_reader
{
    png_structp png;
    png_infop info;
    int handle;
    unsigned width, height;
    int color_type;
    size_t row_bytes;
    unsigned char *pixels;
    png_bytep *rows;
    s_screen *artwork;
    char *error;
    size_t error_capacity;
} meshmap_png_reader;

static void meshmap_png_message(meshmap_png_reader *reader, const char *message)
{
    if(reader->error && reader->error_capacity)
        snprintf(reader->error, reader->error_capacity, "%s", message);
}

/* libpng reports malformed input through longjmp. Mutable ownership lives in
 * this heap record so cleanup does not depend on indeterminate automatic
 * variables after the jump. A rejected replacement never reaches its caller. */
static void meshmap_png_error(png_structp png, png_const_charp message)
{
    meshmap_png_reader *reader = (meshmap_png_reader *)png_get_error_ptr(png);
    if(reader) meshmap_png_message(reader, message);
    png_longjmp(png, 1);
}

static void meshmap_png_warning(png_structp png, png_const_charp message)
{
    (void)png;
    (void)message;
}

/* The pack API bounds reads to the selected logical archive member. Split a
 * libpng request only to satisfy the API's signed-int read length; every
 * short read is a decoding error, including a truncated chunk or scanline. */
static void meshmap_png_read(png_structp png, png_bytep destination, png_size_t bytes)
{
    meshmap_png_reader *reader = (meshmap_png_reader *)png_get_io_ptr(png);
    while(bytes)
    {
        int amount = bytes > INT_MAX ? INT_MAX : (int)bytes;
        if(readpackfile(reader->handle, destination, amount) != amount)
            png_error(png, "Truncated meshmap PNG");
        destination += amount;
        bytes -= (png_size_t)amount;
    }
}

static void meshmap_png_release(meshmap_png_reader *reader)
{
    if(!reader) return;
    if(reader->png) png_destroy_read_struct(&reader->png, &reader->info, NULL);
    if(reader->handle >= 0) closepackfile(reader->handle);
    free(reader->pixels);
    free(reader->rows);
    if(reader->artwork) freescreen(&reader->artwork);
    free(reader);
}

/* Source artwork must preserve arbitrary PNG widths. allocscreen() retains
 * an older four-pixel width truncation, so allocate the ordinary screen layout
 * exactly here. Its palette is independently aligned after the index bytes. */
static s_screen *meshmap_image_screen(unsigned width, unsigned height, int indexed)
{
    size_t bytes, palette_offset, total;
    s_screen *screen;
    if(!width || !height || width > INT_MAX || height > INT_MAX ||
       (size_t)width > SIZE_MAX / height) return NULL;
    bytes = (size_t)width * height;
    if(!indexed)
    {
        if(bytes > SIZE_MAX / sizeof(uint32_t)) return NULL;
        bytes *= sizeof(uint32_t);
    }
    if(bytes > SIZE_MAX - sizeof(*screen) - 3u - 256u * sizeof(uint32_t)) return NULL;
    palette_offset = (bytes + 3u) & ~(size_t)3u;
    total = sizeof(*screen) + (indexed ? palette_offset + 256u * sizeof(uint32_t) : bytes);
    screen = (s_screen *)calloc(1, total);
    if(!screen) return NULL;
    screen->magic = screen_magic;
    screen->width = (int)width;
    screen->height = (int)height;
    screen->pixelformat = indexed ? PIXEL_x8 : PIXEL_32;
    screen->palette = indexed ? screen->data + palette_offset : NULL;
    return screen;
}

/* Read raw rows without palette expansion, gamma correction, or 16-bit
 * stripping. Those transformations are useful for viewing pictures but would
 * change terrain values. Adam7 is handled by libpng using the same full rows. */
static meshmap_png_reader *meshmap_png_load(const char *filename, const char *packfilename,
                                           int geometry, char *error, size_t error_capacity)
{
    meshmap_png_reader *reader;
    if(error && error_capacity) error[0] = '\0';
    if(!filename || !filename[0])
    {
        if(error && error_capacity) snprintf(error, error_capacity, "Meshmap PNG path is empty");
        return NULL;
    }
    reader = (meshmap_png_reader *)calloc(1, sizeof(*reader));
    if(!reader)
    {
        if(error && error_capacity) snprintf(error, error_capacity, "Out of memory loading meshmap PNG");
        return NULL;
    }
    reader->handle = -1;
    reader->error = error;
    reader->error_capacity = error_capacity;
    reader->handle = openpackfile(filename, packfilename);
    if(reader->handle < 0)
    {
        meshmap_png_message(reader, "Cannot open meshmap PNG");
        meshmap_png_release(reader);
        return NULL;
    }
    reader->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, reader,
                                          meshmap_png_error, meshmap_png_warning);
    if(!reader->png)
    {
        meshmap_png_message(reader, "Cannot allocate PNG decoder");
        meshmap_png_release(reader);
        return NULL;
    }
    if(setjmp(png_jmpbuf(reader->png)))
    {
        meshmap_png_release(reader);
        return NULL;
    }
    reader->info = png_create_info_struct(reader->png);
    if(!reader->info) png_error(reader->png, "Cannot allocate PNG information");
    png_set_read_fn(reader->png, reader, meshmap_png_read);
    png_set_crc_action(reader->png, PNG_CRC_ERROR_QUIT, PNG_CRC_ERROR_QUIT);
    png_read_info(reader->png, reader->info);
    {
        png_uint_32 width, height;
        int depth, color, interlace;
        size_t expected, bytes_per_pixel;
        unsigned row;
        png_get_IHDR(reader->png, reader->info, &width, &height, &depth,
                     &color, &interlace, NULL, NULL);
        if(width > INT_MAX || height > INT_MAX || !width || !height ||
           (geometry && (width < 2 || height < 2)))
            png_error(reader->png, "Invalid meshmap PNG dimensions");
        if(geometry ? (depth != 16 || color != PNG_COLOR_TYPE_GRAY)
                    : (depth != 8 || (color != PNG_COLOR_TYPE_PALETTE && color != PNG_COLOR_TYPE_RGB)))
            png_error(reader->png, geometry ? "Geometry PNG must be single-channel grayscale16"
                                            : "Artwork PNG must be indexed8 or RGB24");
        reader->width = (unsigned)width;
        reader->height = (unsigned)height;
        reader->color_type = color;
        bytes_per_pixel = geometry ? 2u : color == PNG_COLOR_TYPE_RGB ? 3u : 1u;
        if((size_t)width > SIZE_MAX / bytes_per_pixel)
            png_error(reader->png, "Meshmap PNG row size overflows");
        expected = (size_t)width * bytes_per_pixel;
        png_set_interlace_handling(reader->png);
        png_read_update_info(reader->png, reader->info);
        reader->row_bytes = png_get_rowbytes(reader->png, reader->info);
        if(reader->row_bytes != expected || reader->row_bytes > SIZE_MAX / height ||
           (size_t)height > SIZE_MAX / sizeof(*reader->rows))
            png_error(reader->png, "Meshmap PNG allocation size overflows");
        reader->pixels = (unsigned char *)malloc(reader->row_bytes * height);
        reader->rows = (png_bytep *)malloc((size_t)height * sizeof(*reader->rows));
        if(!reader->pixels || !reader->rows) png_error(reader->png, "Out of memory reading meshmap PNG");
        for(row = 0; row < height; ++row)
            reader->rows[row] = reader->pixels + (size_t)row * reader->row_bytes;
        png_read_image(reader->png, reader->rows);
        png_read_end(reader->png, reader->info);
    }
    return reader;
}

int meshmap_image_load_samples(const char *filename, const char *packfilename,
                              uint16_t **samples, unsigned *width, unsigned *height,
                              char *error, size_t error_capacity)
{
    meshmap_png_reader *reader;
    uint16_t *decoded;
    size_t index, count;
    if(!samples || !width || !height) return 0;
    reader = meshmap_png_load(filename, packfilename, 1, error, error_capacity);
    if(!reader) return 0;
    decoded = (uint16_t *)reader->pixels;
    count = (size_t)reader->width * reader->height;
    for(index = 0; index < count; ++index)
    {
        unsigned high = reader->pixels[index * 2];
        unsigned low = reader->pixels[index * 2 + 1];
        decoded[index] = (uint16_t)((high << 8) | low);
    }
    *samples = decoded;
    *width = reader->width;
    *height = reader->height;
    reader->pixels = NULL;
    meshmap_png_release(reader);
    return 1;
}

int meshmap_image_load_artwork(const char *filename, const char *packfilename,
                              s_screen **artwork, char *error, size_t error_capacity)
{
    meshmap_png_reader *reader;
    size_t index, count;
    int indexed;
    if(!artwork) return 0;
    reader = meshmap_png_load(filename, packfilename, 0, error, error_capacity);
    if(!reader) return 0;
    indexed = reader->color_type == PNG_COLOR_TYPE_PALETTE;
    reader->artwork = meshmap_image_screen(reader->width, reader->height, indexed);
    if(!reader->artwork)
    {
        meshmap_png_message(reader, "Cannot allocate meshmap artwork");
        meshmap_png_release(reader);
        return 0;
    }
    count = (size_t)reader->width * reader->height;
    if(indexed)
    {
        png_colorp colors;
        int color_count, color;
        uint32_t *palette = (uint32_t *)reader->artwork->palette;
        if(!png_get_PLTE(reader->png, reader->info, &colors, &color_count) ||
           color_count <= 0 || color_count > 256)
        {
            meshmap_png_message(reader, "Indexed artwork has no valid palette");
            meshmap_png_release(reader);
            return 0;
        }
        for(index = 0; index < count; ++index)
        {
            if(reader->pixels[index] >= color_count)
            {
                meshmap_png_message(reader, "Indexed artwork references an absent palette entry");
                meshmap_png_release(reader);
                return 0;
            }
        }
        memcpy(reader->artwork->data, reader->pixels, count);
        for(color = 0; color < color_count; ++color)
            palette[color] = colour32(colors[color].red, colors[color].green, colors[color].blue);
    }
    else
    {
        uint32_t *pixels = (uint32_t *)reader->artwork->data;
        for(index = 0; index < count; ++index)
            pixels[index] = colour32(reader->pixels[index * 3], reader->pixels[index * 3 + 1],
                                     reader->pixels[index * 3 + 2]);
    }
    *artwork = reader->artwork;
    reader->artwork = NULL;
    meshmap_png_release(reader);
    return 1;
}

/* Level sheets and property setters share these transactions. The core copies
 * validated sample data before publishing a new revision. Its artwork setter
 * takes ownership only after the replacement path has also been allocated. */
int meshmap_image_replace_geometry(s_meshmap *map, const char *filename,
                                   const char *packfilename, char *error, size_t error_capacity)
{
    uint16_t *samples = NULL;
    unsigned width, height;
    int result;
    if(!map || !meshmap_image_load_samples(filename, packfilename, &samples,
                                           &width, &height, error, error_capacity)) return 0;
    result = meshmap_set_geometry(map, width, height, samples, filename);
    free(samples);
    if(!result && error && error_capacity)
        snprintf(error, error_capacity, "Cannot replace meshmap geometry");
    return result;
}

static void meshmap_image_release(void *artwork)
{
    s_screen *screen = (s_screen *)artwork;
    freescreen(&screen);
}

int meshmap_image_replace_artwork(s_meshmap *map, const char *filename,
                                  const char *packfilename, char *error, size_t error_capacity)
{
    s_screen *artwork = NULL;
    int result;
    if(error && error_capacity) error[0] = '\0';
    if(!map || !filename) return 0;
    if(filename[0] && !meshmap_image_load_artwork(filename, packfilename, &artwork,
                                                 error, error_capacity)) return 0;
    result = meshmap_set_artwork(map, artwork, meshmap_image_release, filename);
    if(!result)
    {
        if(artwork) freescreen(&artwork);
        if(error && error_capacity) snprintf(error, error_capacity, "Cannot replace meshmap artwork");
    }
    return result;
}
