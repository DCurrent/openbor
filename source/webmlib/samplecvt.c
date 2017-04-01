/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2015 OpenBOR Team
 */

/* This code was derived from code carrying the following copyright notice:
 *
 * Copyright (c) 2002-2015 Xiph.org Foundation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "samplecvt.h"

#if TREMOR

static inline ogg_int32_t CLIP_TO_15(ogg_int32_t x)
{
    int ret = x;
    ret -= ((x<=32767)-1)&(x-32767);
    ret -= ((x>=-32768)-1)&(x+32768);
    return ret;
}

void pack_samples(ogg_int32_t **pcm, short *buffer, int samples, int channels)
{
    int i, j;
    for (i=0; i<channels; i++)
    {
        ogg_int32_t *src = pcm[i];
        short *dest = buffer + i;
        for (j=0; j<samples; j++)
        {
            *dest = CLIP_TO_15(src[j]>>9);
            dest += channels;
        }
    }
}

#else // libvorbis

#include "vorbisfpu.h"

void pack_samples(float **pcm, short *buffer, int samples, int channels)
{
    int i, j;
    vorbis_fpu_control fpu;
    vorbis_fpu_setround(&fpu);
    for(i=0; i<channels; i++)
    {
        float *src = pcm[i];
        short *dest = buffer + i;
        for(j=0; j<samples; j++)
        {
            int val = vorbis_ftoi(src[j]*32768.f);
            if(val > 32767) val = 32767;
            else if(val < -32768) val = -32768;
            *dest = val;
            dest += channels;
        }
    }
    vorbis_fpu_restore(fpu);
}

#endif

