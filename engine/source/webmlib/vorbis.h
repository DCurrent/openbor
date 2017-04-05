/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#ifndef VORBIS_H
#define VORBIS_H

#include <stdlib.h>

#if TREMOR
#include <tremor/ivorbiscodec.h>
#else
#include <vorbis/codec.h>
#endif

typedef struct {
    vorbis_info vi;
    vorbis_comment vc;
    vorbis_dsp_state v;
    vorbis_block vb;
    int channels;
} vorbis_context;

void vorbis_init(vorbis_context *ctx);
void vorbis_destroy(vorbis_context *ctx);
void vorbis_prepare(vorbis_context *ctx);
void vorbis_headerpacket(vorbis_context *ctx, void *data, size_t size, int packetCount);
int vorbis_packet(vorbis_context *ctx, void *data, size_t size);
void vorbis_getpcm(vorbis_context *ctx, void *buffer, size_t samples);

#endif
