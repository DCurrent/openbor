/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) OpenBOR Team
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "packfile.h"
#include "borendian.h"
#include "types.h"
#include "screen.h"
#include "anigif.h"
#include "gpu.h"

/*
 * Damon V. Caskey
 * 2026-11-20
 * GIF compression remains a CPU operation. Its decoded indices are staged in
 * reusable byte buffers; palette expansion, persistent frame composition and
 * the optional RGB stream merge write only native RGB32 render targets.
 */
enum
{
    ANIGIF_PALETTE_COLORS = 256,
    ANIGIF_PALETTE_COMPONENTS = 3,
    ANIGIF_PIXEL_BYTES = 4,
    ANIGIF_BLOCK_BYTES = 255,
    ANIGIF_CONTROL_BYTES = 4,
    ANIGIF_CONTROL_FLAGS = 0,
    ANIGIF_CONTROL_DELAY_LOW = 1,
    ANIGIF_CONTROL_DELAY_HIGH = 2,
    ANIGIF_CONTROL_KEY = 3,
    ANIGIF_LZW_MIN_BITS = 2,
    ANIGIF_LZW_LITERAL_BITS = 8,
    ANIGIF_LZW_MAX_BITS = 12,
    ANIGIF_LZW_CODES = 1 << ANIGIF_LZW_MAX_BITS,
    ANIGIF_LZW_RESERVED_CODES = 2,
    ANIGIF_COLOR_TABLE = 0x80,
    ANIGIF_INTERLACED = 0x40,
    ANIGIF_COLOR_BITS_MASK = 0x07,
    ANIGIF_CONTROL_EXTENSION = 0xf9,
    ANIGIF_CONTROL_TRANSPARENT = 0x01,
    ANIGIF_DELAY_MILLISECONDS = 10,
    ANIGIF_INTERLACE_PASSES = 4,
    ANIGIF_INTERLACE_FIRST_STEP = 8,
    ANIGIF_SIGNATURE_BYTES = 3,
    ANIGIF_EXTENSION_LENGTH = 4,
    ANIGIF_SUFFIX_CHANNEL_OFFSET = 0,
    ANIGIF_RED = 0,
    ANIGIF_GREEN = 1,
    ANIGIF_BLUE = 2
};

typedef struct anigif_lzw_reader
{
    int handle;
    unsigned char block[ANIGIF_BLOCK_BYTES];
    unsigned int accumulator;
    unsigned int bit_count;
    unsigned int block_size;
    unsigned int block_position;
    int terminated;
    int failed;
} anigif_lzw_reader;

typedef struct anigif_dictionary
{
    unsigned short prefix[ANIGIF_LZW_CODES];
    unsigned char suffix[ANIGIF_LZW_CODES];
    unsigned char stack[ANIGIF_LZW_CODES];
} anigif_dictionary;

/*
 * Damon V. Caskey
 * 2026-11-20
 * Distinguish a real zero subblock terminator from a truncated input stream.
 */
static int readbyte(int handle)
{
    unsigned char byte;
    return readpackfile(handle, &byte, 1) == 1 ? byte : -1;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Consume complete extension/image subblocks, rejecting truncated payloads.
 */
static int passgifblock(int handle)
{
    unsigned char block[ANIGIF_BLOCK_BYTES];
    int length;

    while((length = readbyte(handle)) > 0)
    {
        if(readpackfile(handle, block, length) != length)
        {
            return 0;
        }
    }
    return length == 0;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Preserve OpenBOR's control semantics: a set transparency flag replaces the
 * key; an unset flag leaves the previous key, and disposal remains ignored.
 * Validate the control payload before accessing its delay and key fields.
 */
static int handle_gfx_control(anigif_info *info, int n)
{
    unsigned char block[ANIGIF_BLOCK_BYTES];
    int length;

    while((length = readbyte(info->info[n].handle)) > 0)
    {
        if(length < ANIGIF_CONTROL_BYTES ||
           readpackfile(info->info[n].handle, block, length) != length)
        {
            return 0;
        }
        if(block[ANIGIF_CONTROL_FLAGS] & ANIGIF_CONTROL_TRANSPARENT)
        {
            info->info[n].transparent = block[ANIGIF_CONTROL_KEY];
        }
        info->info[n].lastdelay =
            (block[ANIGIF_CONTROL_DELAY_HIGH] << ANIGIF_LZW_LITERAL_BITS) |
            block[ANIGIF_CONTROL_DELAY_LOW];
    }
    return length == 0;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Route graphic controls and skip unrelated GIF extensions without changing
 * the existing playback model or introducing disposal/background operations.
 */
static int gifextension(anigif_info *info, int n)
{
    int function = readbyte(info->info[n].handle);

    if(function < 0)
    {
        return 0;
    }
    if(function == ANIGIF_CONTROL_EXTENSION)
    {
        return handle_gfx_control(info, n);
    }
    return passgifblock(info->info[n].handle);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Read LZW bytes from length-prefixed GIF subblocks. The file position always
 * follows the entire current block, so an end code can drain later blocks.
 */
static int anigif_data_byte(anigif_lzw_reader *reader)
{
    int length;

    if(reader->block_position == reader->block_size)
    {
        length = readbyte(reader->handle);
        if(length <= 0)
        {
            reader->terminated = length == 0;
            reader->failed = length < 0;
            return -1;
        }
        if(readpackfile(reader->handle, reader->block, length) != length)
        {
            reader->failed = 1;
            return -1;
        }
        reader->block_size = (unsigned int)length;
        reader->block_position = 0;
    }
    return reader->block[reader->block_position++];
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Reassemble little-endian variable-width codes without reading beyond a
 * subblock or carrying undefined bits into the next dictionary lookup.
 */
static int anigif_lzw_code(anigif_lzw_reader *reader, unsigned int width)
{
    unsigned int code;
    int byte;

    while(reader->bit_count < width)
    {
        byte = anigif_data_byte(reader);
        if(byte < 0)
        {
            return -1;
        }
        reader->accumulator |= (unsigned int)byte << reader->bit_count;
        reader->bit_count += ANIGIF_LZW_LITERAL_BITS;
    }
    code = reader->accumulator & ((1u << width) - 1u);
    reader->accumulator >>= width;
    reader->bit_count -= width;
    return (int)code;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Decode a complete indexed frame before publishing any pixels. Narrow
 * interlaced images skip empty passes; malformed chains, surplus pixels and
 * truncated payloads cannot leave a partly initialized GPU upload rectangle.
 * A proper subblock terminator after all pixels retains the old decoder's
 * tolerance for streams that omit an explicit LZW end code.
 */
static int decodegifblock(anigif_info *info, const gifblockstruct *block, int n)
{
    anigif_lzw_reader reader = {0};
    anigif_dictionary dictionary;
    unsigned char *indices;
    size_t pixels, written = 0;
    unsigned int clear_code, end_code, next_code, code_width;
    unsigned int code, incoming, first = 0, stack_size;
    unsigned int column = 0, row = 0, pass = 0, step = ANIGIF_INTERLACE_FIRST_STEP;
    int minimum_bits, old_code = NO_CODE, value;

    if(!block->width || !block->height)
    {
        return 0;
    }
    if((size_t)block->width > SIZE_MAX / block->height)
    {
        return 0;
    }
    pixels = (size_t)block->width * block->height;
    if(info->info[n].indices_capacity < pixels)
    {
        indices = realloc(info->info[n].indices, pixels);
        if(!indices)
        {
            return 0;
        }
        info->info[n].indices = indices;
        info->info[n].indices_capacity = pixels;
    }
    indices = info->info[n].indices;
    reader.handle = info->info[n].handle;
    minimum_bits = readbyte(reader.handle);
    if(minimum_bits < ANIGIF_LZW_MIN_BITS || minimum_bits > ANIGIF_LZW_LITERAL_BITS)
    {
        return 0;
    }
    clear_code = 1u << minimum_bits;
    end_code = clear_code + 1;
    next_code = clear_code + ANIGIF_LZW_RESERVED_CODES;
    code_width = (unsigned int)minimum_bits + 1;

    for(;;)
    {
        value = anigif_lzw_code(&reader, code_width);
        if(value < 0)
        {
            return reader.terminated && !reader.failed && written == pixels;
        }
        code = incoming = (unsigned int)value;
        if(code == clear_code)
        {
            next_code = clear_code + ANIGIF_LZW_RESERVED_CODES;
            code_width = (unsigned int)minimum_bits + 1;
            old_code = NO_CODE;
            continue;
        }
        if(code == end_code)
        {
            return written == pixels && passgifblock(reader.handle);
        }
        if(code > next_code || code >= ANIGIF_LZW_CODES)
        {
            return 0;
        }
        stack_size = 0;
        if(code == next_code)
        {
            if(old_code == NO_CODE)
            {
                return 0;
            }
            dictionary.stack[stack_size++] = (unsigned char)first;
            code = (unsigned int)old_code;
        }
        while(code >= clear_code)
        {
            if(code < clear_code + ANIGIF_LZW_RESERVED_CODES || code >= next_code ||
               stack_size >= ANIGIF_LZW_CODES)
            {
                return 0;
            }
            dictionary.stack[stack_size++] = dictionary.suffix[code];
            code = dictionary.prefix[code];
        }
        first = code;
        if(stack_size >= ANIGIF_LZW_CODES)
        {
            return 0;
        }
        dictionary.stack[stack_size++] = (unsigned char)first;
        while(stack_size)
        {
            if(written >= pixels || row >= block->height)
            {
                return 0;
            }
            indices[(size_t)row * block->width + column] = dictionary.stack[--stack_size];
            ++written;
            if(++column == block->width)
            {
                column = 0;
                if(block->flags & ANIGIF_INTERLACED)
                {
                    row += step;
                    while(row >= block->height && pass < ANIGIF_INTERLACE_PASSES)
                    {
                        ++pass;
                        if(pass == 1)
                        {
                            row = ANIGIF_INTERLACE_FIRST_STEP / 2;
                        }
                        else
                        {
                            step /= 2;
                            row = step / 2;
                        }
                    }
                }
                else
                {
                    ++row;
                }
            }
        }
        if(old_code != NO_CODE && next_code < ANIGIF_LZW_CODES)
        {
            dictionary.prefix[next_code] = (unsigned short)old_code;
            dictionary.suffix[next_code] = (unsigned char)first;
            ++next_code;
            if(next_code == (1u << code_width) && code_width < ANIGIF_LZW_MAX_BITS)
            {
                ++code_width;
            }
        }
        old_code = (int)incoming;
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Retire render targets through normal screen coherence hooks, then release
 * palettes and reusable LZW storage. Closing a partially opened GIF is safe.
 */
void anigif_close(anigif_info *info)
{
    int n;

    if(!info)
    {
        return;
    }
    for(n = 0; n < ANIGIF_CHANNEL_COUNT; ++n)
    {
        if(info->info[n].handle >= 0)
        {
            closepackfile(info->info[n].handle);
            info->info[n].handle = -1;
        }
        freescreen(&info->gifbuffer[n]);
        free(info->info[n].global_pal);
        free(info->info[n].local_pal);
        free(info->info[n].indices);
        info->info[n].global_pal = NULL;
        info->info[n].local_pal = NULL;
        info->info[n].indices = NULL;
        info->info[n].indices_capacity = 0;
    }
    freescreen(&info->backbuffer);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Convert at most 256 palette entries once per table. This is source metadata;
 * per-pixel palette lookup is performed by the native composition shader.
 * Unused entries are deterministic black, and ordinary GIFs retain OpenBOR's
 * established black-at-index-zero convention. RGB triplets keep every color.
 */
static int anigif_read_palette(int handle, unsigned char **destination, int colors, int rgb)
{
    unsigned char source[ANIGIF_PALETTE_COLORS * ANIGIF_PALETTE_COMPONENTS] = {0};
    unsigned int *palette;
    int color;

    if(!*destination)
    {
        *destination = calloc(1, PAL_BYTES);
        if(!*destination)
        {
            return 0;
        }
    }
    if(readpackfile(handle, source, colors * ANIGIF_PALETTE_COMPONENTS) !=
       colors * ANIGIF_PALETTE_COMPONENTS)
    {
        return 0;
    }
    if(!rgb)
    {
        source[0] = source[1] = source[2] = 0;
    }
    palette = (unsigned int *)*destination;
    for(color = 0; color < ANIGIF_PALETTE_COLORS; ++color)
    {
        int offset = color * ANIGIF_PALETTE_COMPONENTS;
        palette[color] = colour32(source[offset], source[offset + 1], source[offset + 2]);
    }
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Validate the stream header and read its optional global palette. Native
 * canvas allocation is deferred until every requested channel is open.
 */
static int test_anigif_open(anigif_info *info, char *name, char *packfilename, int n)
{
    gifheaderstruct *header = &info->info[n].gif_header;

    info->info[n].handle = openpackfile(name, packfilename);
    if(info->info[n].handle < 0 ||
       readpackfile(info->info[n].handle, header, sizeof_gifheaderstruct) != sizeof_gifheaderstruct ||
       memcmp(header->magic, "GIF", ANIGIF_SIGNATURE_BYTES))
    {
        return 0;
    }
    header->screenwidth = SwapLSB16(header->screenwidth);
    header->screenheight = SwapLSB16(header->screenheight);
    if(!header->screenwidth || !header->screenheight)
    {
        return 0;
    }
    info->info[n].bitdepth = (header->flags & ANIGIF_COLOR_TABLE) ?
        (header->flags & ANIGIF_COLOR_BITS_MASK) + 1 : ANIGIF_LZW_LITERAL_BITS;
    info->info[n].numcolours = (header->flags & ANIGIF_COLOR_TABLE) ?
        1 << info->info[n].bitdepth : 0;
    info->info[n].lastdelay = 1;
    info->info[n].code = ANIGIF_DECODE_RETRY;
    return !(header->flags & ANIGIF_COLOR_TABLE) ||
        anigif_read_palette(info->info[n].handle, &info->info[n].global_pal,
                           info->info[n].numcolours, info->isRGB);
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Keep GIF logical dimensions exact: general allocscreen() rounds widths down
 * to four pixels, which cannot represent narrow GIF canvases. Allocate the
 * normal RGB32 screen layout with checked size arithmetic and initialize its
 * image on the GPU; its CPU shadow stays untouched until explicit readback.
 */
static s_screen *anigif_canvas(unsigned int width, unsigned int height)
{
    s_screen *screen;
    size_t pixels, bytes;

    if(!width || !height || (size_t)width > SIZE_MAX / height)
    {
        return NULL;
    }
    pixels = (size_t)width * height;
    if(pixels > (SIZE_MAX - sizeof(*screen)) / ANIGIF_PIXEL_BYTES)
    {
        return NULL;
    }
    bytes = sizeof(*screen) + pixels * ANIGIF_PIXEL_BYTES;
    screen = malloc(bytes);
    if(!screen)
    {
        return NULL;
    }
    screen->magic = screen_magic;
    screen->width = (int)width;
    screen->height = (int)height;
    screen->pixelformat = PIXEL_32;
    screen->palette = NULL;
    if(!gpu_clear_screen(screen))
    {
        gpu_log("ERROR: Animated GIF native canvas initialization failed; GIF stopped without a CPU retry.");
        freescreen(&screen);
    }
    return screen;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Name the optional RGB component files without a fixed filename-size limit.
 */
static char anigif_channel_name(int n)
{
    switch(n)
    {
    case ANIGIF_RED: return 'r';
    case ANIGIF_GREEN: return 'g';
    default: return 'b';
    }
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Open the existing single-GIF or indexed-asset RGB-triplet format. Every
 * public image remains a persistent RGB32 screen and therefore keeps script
 * drawing, explicit CPU reads, frame timing and resource ownership intact.
 */
int anigif_open(char *filename, char *packfilename, anigif_info *info)
{
    char *name = NULL;
    size_t length, stem_length;
    int n, channels;
    const char suffix[] = "r.gif";

    if(!info)
    {
        return 0;
    }
    memset(info, 0, sizeof(*info));
    for(n = 0; n < ANIGIF_CHANNEL_COUNT; ++n)
    {
        info->info[n].handle = -1;
        info->info[n].transparent = NO_CODE;
    }
    if(!filename)
    {
        return 0;
    }
    length = strlen(filename);
    stem_length = length;
    if(length >= ANIGIF_EXTENSION_LENGTH &&
       stricmp(filename + length - ANIGIF_EXTENSION_LENGTH, ".gif") == 0)
    {
        stem_length -= ANIGIF_EXTENSION_LENGTH;
    }
    if(stem_length > SIZE_MAX - sizeof(suffix))
    {
        return 0;
    }
    name = malloc(stem_length + sizeof(suffix));
    if(!name)
    {
        return 0;
    }
    memcpy(name, filename, stem_length);
    memcpy(name + stem_length, suffix, sizeof(suffix));

    /* Source artwork mode remains independent of the RGB32 scene format. */
    info->isRGB = pixelformat == PIXEL_x8;
    if(info->isRGB)
    {
        for(n = 0; n < ANIGIF_CHANNEL_COUNT; ++n)
        {
            name[stem_length + ANIGIF_SUFFIX_CHANNEL_OFFSET] = anigif_channel_name(n);
            if(testpackfile(name, packfilename) < 0)
            {
                info->isRGB = 0;
                break;
            }
        }
    }
    channels = info->isRGB ? ANIGIF_CHANNEL_COUNT : 1;
    for(n = 0; n < channels; ++n)
    {
        name[stem_length + ANIGIF_SUFFIX_CHANNEL_OFFSET] = anigif_channel_name(n);
        if(!test_anigif_open(info, info->isRGB ? name : filename, packfilename, n))
        {
            goto failed;
        }
    }
    free(name);
    name = NULL;

    for(n = 0; n < channels; ++n)
    {
        info->gifbuffer[n] = anigif_canvas(info->info[0].gif_header.screenwidth,
                                         info->info[0].gif_header.screenheight);
        if(!info->gifbuffer[n])
        {
            goto failed;
        }
    }
    if(info->isRGB)
    {
        info->backbuffer = anigif_canvas(info->info[0].gif_header.screenwidth,
                                        info->info[0].gif_header.screenheight);
        if(!info->backbuffer)
        {
            goto failed;
        }
    }
    info->frame = -1;
    return 1;

failed:
    free(name);
    anigif_close(info);
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Decode one record into indices, then publish a complete frame through the
 * palette shader. Transparency preserves resident destination pixels, even
 * after scripts have drawn into that public canvas. Failed decoding or GPU
 * composition ends this stream without CPU rendering or a partial upload.
 */
int anigif_decode(anigif_info *info, int n)
{
    gifblockstruct block;
    gpu_indexed_frame frame;
    int record, colors;
    const unsigned char *palette;

    if(info->info[n].handle < 0 || !info->gifbuffer[n])
    {
        return ANIGIF_DECODE_END;
    }
    record = readbyte(info->info[n].handle);
    switch(record)
    {
    case ',':
        if(readpackfile(info->info[n].handle, &block, sizeof_iblock) != sizeof_iblock)
        {
            goto failed;
        }
        if(block.flags & ANIGIF_COLOR_TABLE)
        {
            colors = 1 << ((block.flags & ANIGIF_COLOR_BITS_MASK) + 1);
            if(!anigif_read_palette(info->info[n].handle, &info->info[n].local_pal,
                                   colors, info->isRGB))
            {
                goto failed;
            }
        }
        else
        {
            free(info->info[n].local_pal);
            info->info[n].local_pal = NULL;
        }
        block.left = SwapLSB16(block.left);
        block.top = SwapLSB16(block.top);
        block.width = SwapLSB16(block.width);
        block.height = SwapLSB16(block.height);
        palette = info->info[n].local_pal ? info->info[n].local_pal : info->info[n].global_pal;
        if(!palette || !decodegifblock(info, &block, n))
        {
            goto failed;
        }
        frame.indices = info->info[n].indices;
        frame.palette = palette;
        frame.width = block.width;
        frame.height = block.height;
        frame.x = block.left;
        frame.y = block.top;
        frame.transparent = info->info[n].transparent;
        if(!gpu_compose_indexed(info->gifbuffer[n], &frame))
        {
            gpu_log("ERROR: Animated GIF native frame composition failed on channel %d; GIF stopped without a CPU retry.", n);
            return ANIGIF_DECODE_END;
        }
        return ANIGIF_DECODE_FRAME | ((block.flags & ANIGIF_COLOR_TABLE) ? ANIGIF_DECODE_PAL : 0);

    case '!':
        if(!gifextension(info, n))
        {
            goto failed;
        }
        return ANIGIF_DECODE_RETRY;

    case -1:
    case ';':
        return ANIGIF_DECODE_END;

    default:
        return ANIGIF_DECODE_RETRY;
    }

failed:
    gpu_log("ERROR: Animated GIF frame data is incomplete or invalid on channel %d; GIF stopped before frame composition.", n);
    return ANIGIF_DECODE_END;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Preserve the original bitwise RGB union on resident channel textures,
 * including arbitrary script edits to the public RGB32 channel screens.
 */
static int anigif_updatebuffer(anigif_info *info)
{
    if(gpu_merge_channels(info->backbuffer, info->gifbuffer[ANIGIF_RED],
                          info->gifbuffer[ANIGIF_GREEN], info->gifbuffer[ANIGIF_BLUE]))
    {
        return 1;
    }
    gpu_log("ERROR: Animated GIF native RGB merge failed; GIF stopped without a CPU retry.");
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Advance channels together and retain their existing centisecond timing.
 * The frame number advances only after all palette draws and any RGB merge
 * succeed; ending any component ends the combined animation as before.
 */
int anigif_decode_frame(anigif_info *info)
{
    int n, channels = info->isRGB ? ANIGIF_CHANNEL_COUNT : 1;

    if(!info->done)
    {
        for(n = 0; n < channels; ++n)
        {
            if(info->info[n].code != ANIGIF_DECODE_END)
            {
                do
                {
                    info->info[n].code = anigif_decode(info, n);
                }
                while(info->info[n].code == ANIGIF_DECODE_RETRY);
                info->info[n].nextframe += info->info[n].lastdelay * ANIGIF_DELAY_MILLISECONDS;
            }
            if(info->info[n].code == ANIGIF_DECODE_END)
            {
                info->done = 1;
                break;
            }
        }
        if(!info->done)
        {
            if(info->isRGB && !anigif_updatebuffer(info))
            {
                info->done = 1;
            }
            else
            {
                ++info->frame;
            }
        }
    }
    return info->done;
}

/*
 * Damon V. Caskey
 * 2026-11-20
 * Return the same persistent public screen without resolving its CPU shadow.
 * Explicit pixel readers still trigger synchronization through normal screen
 * APIs; GPU consumers can sample the current image without a round trip.
 */
s_screen *anigif_getbuffer(anigif_info *info)
{
    s_screen *buffer = info->isRGB ? info->backbuffer : info->gifbuffer[0];

    if(buffer)
    {
        buffer->palette = info->isRGB ? NULL :
            (info->info[0].local_pal ? info->info[0].local_pal : info->info[0].global_pal);
    }
    return buffer;
}
