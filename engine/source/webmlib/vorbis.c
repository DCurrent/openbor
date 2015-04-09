/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "vorbis.h"
#include "samplecvt.h"

void vorbis_init(vorbis_context *ctx)
{
    vorbis_info_init(&ctx->vi);
    vorbis_comment_init(&ctx->vc);
}

void vorbis_destroy(vorbis_context *ctx)
{
    vorbis_block_clear(&ctx->vb);
    vorbis_dsp_clear(&ctx->v);
    vorbis_comment_clear(&ctx->vc);
    vorbis_info_clear(&ctx->vi);
}

// call after reading header packets but before audio data
void vorbis_prepare(vorbis_context *ctx)
{
    if (vorbis_synthesis_init(&ctx->v, &ctx->vi) != 0) exit(1);
    vorbis_block_init(&ctx->v, &ctx->vb);
}

void vorbis_headerpacket(vorbis_context *ctx, void *data, size_t size, int packetCount)
{
    ogg_packet op;
    memset(&op, 0, sizeof(op));
    op.packet = data;
    op.bytes = size;
    op.b_o_s = (packetCount == 0);
    op.e_o_s = 0;
    op.granulepos = 0;
    op.packetno = packetCount;
    
    int result = vorbis_synthesis_headerin(&ctx->vi, &ctx->vc, &op);
    if (result != 0) fprintf(stdout, "vorbis_synthesis_headerin returned %i\n", result);
}

// return number of samples available
int vorbis_packet(vorbis_context *ctx, void *data, size_t size)
{
#if TREMOR
    ogg_int32_t **pcm;
#else
    float **pcm;
#endif

    ogg_packet op;
    memset(&op, 0, sizeof(op));
    op.packet = data;
    op.bytes = size;
    op.b_o_s = 0;
    op.e_o_s = 0;
    op.granulepos = 0;
    op.packetno = 3;

    int result = vorbis_synthesis(&ctx->vb, &op);
    if (result != 0) fprintf(stdout, "vorbis_synthesis returned %i\n", result);
    result = vorbis_synthesis_blockin(&ctx->v, &ctx->vb);
    if (result != 0) fprintf(stdout, "vorbis_synthesis_blockin returned %i\n", result);
    int samples = vorbis_synthesis_pcmout(&ctx->v, &pcm);
    vorbis_synthesis_read(&ctx->v, 0);
    return samples;
}

void vorbis_getpcm(vorbis_context *ctx, void *buffer, size_t samples)
{
#if TREMOR
    ogg_int32_t **pcm;
#else
    float **pcm;
#endif

    if (!samples) return;
    int avail_samples = vorbis_synthesis_pcmout(&ctx->v, &pcm);
    assert(avail_samples >= samples);

    pack_samples(pcm, buffer, samples, ctx->channels);
    vorbis_synthesis_read(&ctx->v, samples);
}



