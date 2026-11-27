/*
 * Damon V. Caskey - 2026-11-27
 * Exercise the production PNG loader with real encoded files, including all
 * grayscale16 boundary values, odd artwork widths, damaged files, and atomic
 * replacements. A tiny filesystem adapter supplies the pack reader contract.
 */
#include <assert.h>
#include <fcntl.h>
#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "meshmap_image.h"

static unsigned screen_releases;
int openpackfile(const char *filename, const char *packfilename)
{
    (void)packfilename;
    return open(filename, O_RDONLY);
}
int readpackfile(int handle, void *buffer, int length)
{
    return (int)read(handle, buffer, (size_t)length);
}
int closepackfile(int handle) { return close(handle); }
void freescreen(s_screen **screen)
{
    if(*screen) ++screen_releases;
    free(*screen);
    *screen = NULL;
}
unsigned colour32(unsigned char r, unsigned char g, unsigned char b)
{
#if REVERSE_COLOR
    return ((unsigned)r << 16) | ((unsigned)g << 8) | b;
#else
    return ((unsigned)b << 16) | ((unsigned)g << 8) | r;
#endif
}

static void write_png(const char *path, unsigned width, unsigned height, int depth,
                       int color, int interlace, const unsigned char *pixels,
                       const png_color *palette, unsigned palette_count)
{
    FILE *file = fopen(path, "wb");
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    png_bytep *rows;
    unsigned row;
    size_t bytes = (size_t)width * (color == PNG_COLOR_TYPE_RGB ? 3 : color == PNG_COLOR_TYPE_RGBA ? 4 : depth / 8);
    assert(file && png && info);
    assert(!setjmp(png_jmpbuf(png)));
    png_init_io(png, file);
    png_set_IHDR(png, info, width, height, depth, color, interlace,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_gAMA(png, info, 0.45455);
    if(palette) png_set_PLTE(png, info, palette, (int)palette_count);
    png_write_info(png, info);
    rows = malloc((size_t)height * sizeof(*rows));
    assert(rows);
    for(row = 0; row < height; ++row) rows[row] = (png_bytep)(pixels + row * bytes);
    png_write_image(png, rows);
    png_write_end(png, info);
    free(rows);
    png_destroy_write_struct(&png, &info);
    fclose(file);
}

static void damage_file(const char *source, const char *destination, int truncate_end)
{
    FILE *input = fopen(source, "rb"), *output;
    long size;
    unsigned char *bytes;
    assert(input);
    assert(!fseek(input, 0, SEEK_END));
    size = ftell(input);
    assert(size > 40 && !fseek(input, 0, SEEK_SET));
    bytes = malloc((size_t)size);
    assert(bytes && fread(bytes, 1, (size_t)size, input) == (size_t)size);
    fclose(input);
    if(truncate_end) --size;
    else bytes[29] ^= 0x80; /* IHDR CRC, leaving the signature valid. */
    output = fopen(destination, "wb");
    assert(output && fwrite(bytes, 1, (size_t)size, output) == (size_t)size);
    fclose(output);
    free(bytes);
}

int main(int argc, char **argv)
{
    static const uint16_t expected[15] = {0, 1, 2, 255, 256, 32767, 32768, 32769,
                                         65534, 65535, 9, 888, 1234, 40000, 1};
    unsigned char raw[30], indices[9] = {0, 1, 2, 2, 1, 0, 1, 2, 0};
    unsigned char rgb[18] = {0, 0, 0, 255, 1, 2, 3, 254, 4, 5, 6, 253, 17, 18, 19, 20, 21, 22};
    png_color palette[3] = {{0, 0, 0}, {250, 17, 25}, {3, 200, 19}};
    char geometry[1024], interlaced[1024], indexed[1024], color[1024], bad[1024], truncated[1024], gray8[1024];
    char error[256];
    uint16_t *samples = NULL;
    unsigned width = 99, height = 99, index, released;
    s_screen *artwork = NULL;
    s_meshmap_store *store;
    s_meshmap *map;
    uint64_t revision;
    uint16_t *old_samples;
    assert(argc == 2);
    snprintf(geometry, sizeof(geometry), "%s/geometry.png", argv[1]);
    snprintf(interlaced, sizeof(interlaced), "%s/geometry-adam7.png", argv[1]);
    snprintf(indexed, sizeof(indexed), "%s/indexed.png", argv[1]);
    snprintf(color, sizeof(color), "%s/rgb.png", argv[1]);
    snprintf(bad, sizeof(bad), "%s/bad-crc.png", argv[1]);
    snprintf(truncated, sizeof(truncated), "%s/truncated.png", argv[1]);
    snprintf(gray8, sizeof(gray8), "%s/gray8.png", argv[1]);
    for(index = 0; index < 15; ++index)
    {
        raw[index * 2] = (unsigned char)(expected[index] >> 8);
        raw[index * 2 + 1] = (unsigned char)expected[index];
    }
    write_png(geometry, 5, 3, 16, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, raw, NULL, 0);
    write_png(interlaced, 5, 3, 16, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_ADAM7, raw, NULL, 0);
    write_png(indexed, 3, 3, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, indices, palette, 3);
    write_png(color, 3, 2, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, rgb, NULL, 0);
    write_png(gray8, 3, 3, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, indices, NULL, 0);
    damage_file(geometry, bad, 0);
    damage_file(geometry, truncated, 1);
    assert(meshmap_image_load_samples(geometry, NULL, &samples, &width, &height, error, sizeof(error)));
    assert(width == 5 && height == 3 && !memcmp(samples, expected, sizeof(expected)));
    free(samples);
    samples = NULL;
    assert(meshmap_image_load_samples(interlaced, NULL, &samples, &width, &height, error, sizeof(error)));
    assert(!memcmp(samples, expected, sizeof(expected)));
    free(samples);
    samples = (uint16_t *)(uintptr_t)1;
    width = height = 99;
    assert(!meshmap_image_load_samples(gray8, NULL, &samples, &width, &height, error, sizeof(error)));
    assert(samples == (uint16_t *)(uintptr_t)1 && width == 99 && height == 99 && error[0]);
    assert(!meshmap_image_load_samples(color, NULL, &samples, &width, &height, error, sizeof(error)));
    assert(!meshmap_image_load_samples(bad, NULL, &samples, &width, &height, error, sizeof(error)));
    assert(!meshmap_image_load_samples(truncated, NULL, &samples, &width, &height, error, sizeof(error)));
    assert(!meshmap_image_load_artwork(geometry, NULL, &artwork, error, sizeof(error)) && !artwork);
    assert(meshmap_image_load_artwork(indexed, NULL, &artwork, error, sizeof(error)));
    assert(artwork->width == 3 && artwork->height == 3 && artwork->pixelformat == PIXEL_x8);
    assert(((uintptr_t)artwork->palette & 3u) == 0 && !memcmp(artwork->data, indices, sizeof(indices)));
    assert(((uint32_t *)artwork->palette)[0] == colour32(0, 0, 0));
    assert(((uint32_t *)artwork->palette)[1] == colour32(250, 17, 25));
    freescreen(&artwork);
    assert(meshmap_image_load_artwork(color, NULL, &artwork, error, sizeof(error)));
    assert(artwork->width == 3 && artwork->height == 2 && artwork->pixelformat == PIXEL_32);
    for(index = 0; index < 6; ++index)
        assert(((uint32_t *)artwork->data)[index] == colour32(rgb[index * 3], rgb[index * 3 + 1], rgb[index * 3 + 2]));
    freescreen(&artwork);
    store = meshmap_store_create();
    assert(store && (map = meshmap_create(store, 0)));
    assert(meshmap_image_replace_geometry(map, geometry, NULL, error, sizeof(error)));
    revision = map->revision;
    old_samples = map->samples;
    assert(!meshmap_image_replace_geometry(map, bad, NULL, error, sizeof(error)));
    assert(map->revision == revision && map->samples == old_samples && !strcmp(map->geometry_path, geometry));
    assert(meshmap_image_replace_artwork(map, indexed, NULL, error, sizeof(error)));
    artwork = map->artwork;
    revision = map->revision;
    assert(!meshmap_image_replace_artwork(map, bad, NULL, error, sizeof(error)));
    assert(map->artwork == artwork && map->revision == revision);
    released = screen_releases;
    assert(meshmap_image_replace_artwork(map, color, NULL, error, sizeof(error)));
    assert(screen_releases == released + 1 && map->artwork != artwork);
    assert(meshmap_image_replace_artwork(map, "", NULL, error, sizeof(error)) && !map->artwork);
    assert(map->samples == old_samples);
    meshmap_store_free(store);
    puts("Meshmap PNG tests passed: raw grayscale16, Adam7, gamma-independent samples, indexed/RGB odd widths, malformed inputs, and atomic resource replacement.");
    return 0;
}
