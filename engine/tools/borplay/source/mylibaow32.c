/*
Mylibaow32 0.1
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org

This is a set of libao compatible functions for using waveOut on
Windows, which unfortunately is not officially supported (I have
seen something only in libao2 in Mplayer).
It has been written enough quickly just for having a fast and
simple audio support in one of my tools so it's not 100% complete.

Most of the code has been written by David Overton in his great
tutorial for using Windows waveOut:

  http://www.insomniavisions.com/documents/tutorials/wave.php

Libao homepage:

  http://www.xiph.org/ao/

---
    Copyright 2006 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl.txt
*/

#include <windows.h>
#include <mmsystem.h>
#include "ao.h"



#define BLOCK_SIZE              8192
#define BLOCK_COUNT             20
#define WAVE_FORMAT_EXTENSIBLE  0xFFFE
typedef struct
{
  WAVEFORMATEX  Format;
  union
  {
    WORD  wValidBitsPerSample;
    WORD  wSamplesPerBlock;
    WORD  wReserved;
  } Samples;
  DWORD  dwChannelMask;
  GUID  SubFormat;
} WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;



static CRITICAL_SECTION waveCriticalSection;
static WAVEHDR*         waveBlocks;
static volatile int     waveFreeBlockCount;
static int              waveCurrentBlock;



static void CALLBACK waveOutProc(
    HWAVEOUT hWaveOut,
    UINT uMsg,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
) {
    int* freeBlockCounter = (int*)dwInstance;

    if(uMsg != WOM_DONE) return;

    EnterCriticalSection(&waveCriticalSection);
    (*freeBlockCounter)++;
    LeaveCriticalSection(&waveCriticalSection);
}



WAVEHDR* allocateBlocks(int size, int count) {
    unsigned char   *buffer;
    int             i;
    WAVEHDR         *blocks;

    buffer = HeapAlloc(
        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        (size + sizeof(WAVEHDR)) * count);
    if(!buffer) {
//        fprintf(stderr, "Memory allocation error\n");
        ExitProcess(1);
    }

    blocks = (WAVEHDR *)buffer;
    buffer += sizeof(WAVEHDR) * count;
    for(i = 0; i < count; i++) {
        blocks[i].dwBufferLength = size;
        blocks[i].lpData = buffer;
        buffer += size;
    }

    return blocks;
}



void freeBlocks(WAVEHDR *blockArray) {
    HeapFree(GetProcessHeap(), 0, blockArray);
}



static ao_info  ao_info_list;



void ao_initialize() {
    ao_info_list.type                  = AO_TYPE_LIVE;
    ao_info_list.name                  = "Windows waveOut";
    ao_info_list.short_name            = "win32";
    ao_info_list.author                = "";
    ao_info_list.comment               = "";
    ao_info_list.preferred_byte_format = AO_FMT_LITTLE;
    ao_info_list.priority              = 20;
    ao_info_list.options               = NULL;
    ao_info_list.option_count          = 0;
}



void ao_shutdown() {
}



void ao_free_options(ao_option *options) {
}



ao_device* ao_open_live(int driver_id, ao_sample_format *format, ao_option *options) {
    HWAVEOUT                hWaveOut;
    WAVEFORMATEX            wfx;
    WAVEFORMATEXTENSIBLE    wfex;
    GUID KSDATAFORMAT_SUBTYPE_PCM = {
        0x00000001,
        0x0000,
        0x0010,
        { 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71 }
    };
    ao_device               *device;

    if(!waveOutGetNumDevs()) return(NULL);

    waveBlocks          = allocateBlocks(BLOCK_SIZE, BLOCK_COUNT);
    waveFreeBlockCount  = BLOCK_COUNT;
    waveCurrentBlock    = 0;

    InitializeCriticalSection(&waveCriticalSection);

    wfx.cbSize          = 0;
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = format->channels;
    wfx.nSamplesPerSec  = format->rate;
    wfx.wBitsPerSample  = format->bits;
    wfx.nBlockAlign     = (wfx.nChannels * wfx.wBitsPerSample) >> 3;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if(waveOutOpen(
      &hWaveOut,
      WAVE_MAPPER,
      &wfx,
      (DWORD_PTR)waveOutProc,
      (DWORD_PTR)&waveFreeBlockCount,
      CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {

        wfex.Format            = wfx;
        wfex.Format.cbSize     = 22;
        wfex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfex.Samples.wReserved = 0;
        wfex.dwChannelMask     = 0;
        wfex.SubFormat         = KSDATAFORMAT_SUBTYPE_PCM;

        if(waveOutOpen(
          &hWaveOut,
          WAVE_MAPPER,
          (WAVEFORMATEX*)&wfex,
          (DWORD_PTR)waveOutProc,
          (DWORD_PTR)&waveFreeBlockCount,
          CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
            return(NULL);
        }
    }

    device = malloc(sizeof(ao_device));
    device->driver_id = hWaveOut;
    return(device);
}



ao_device* ao_open_file(int driver_id, const char *filename, int overwrite, ao_sample_format *format, ao_option *option) {
    return(NULL);
}



int ao_play(ao_device *device, char *output_samples, uint_32 num_bytes) {
    HWAVEOUT    hWaveOut;
    WAVEHDR     *current;
    int         remain;

    hWaveOut = device->driver_id;

    current = &waveBlocks[waveCurrentBlock];

    while(num_bytes > 0) {
        if(current->dwFlags & WHDR_PREPARED)
            waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));

        if(num_bytes < (int)(BLOCK_SIZE - current->dwUser)) {
            memcpy(current->lpData + current->dwUser, output_samples, num_bytes);
            current->dwUser += num_bytes;
            break;
        }

        remain = BLOCK_SIZE - current->dwUser;
        memcpy(current->lpData + current->dwUser, output_samples, remain);
        num_bytes -= remain;
        output_samples += remain;
        current->dwBufferLength = BLOCK_SIZE;

        waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));

        EnterCriticalSection(&waveCriticalSection);
        waveFreeBlockCount--;
        LeaveCriticalSection(&waveCriticalSection);

        while(!waveFreeBlockCount) Sleep(10);

        waveCurrentBlock++;
        waveCurrentBlock %= BLOCK_COUNT;

        current = &waveBlocks[waveCurrentBlock];
        current->dwUser = 0;
    }
    return(1);
}



int ao_close(ao_device *device) {
    HWAVEOUT    hWaveOut;
    int         i;

    hWaveOut = device->driver_id;

    while(waveFreeBlockCount < BLOCK_COUNT) Sleep(10);
    for(i = 0; i < waveFreeBlockCount; i++)
        if(waveBlocks[i].dwFlags & WHDR_PREPARED)
            waveOutUnprepareHeader(hWaveOut, &waveBlocks[i], sizeof(WAVEHDR));
    waveOutReset(hWaveOut);
    freeBlocks(waveBlocks);
    waveOutClose(hWaveOut);
    free(device);
    return(1);
}



int ao_driver_id(const char *short_name) {
    return(0);
}



int ao_default_driver_id(void) {
    return(0);
}



ao_info *ao_driver_info(int driver_id) {
    return(&ao_info_list);
}



ao_info **ao_driver_info_list(int *driver_count) {
    static ao_info  *ret_list[1];

    *driver_count = 1;

    ret_list[0] = &ao_info_list;
    return(ret_list);
}



char *ao_file_extension(int driver_id) {
    return("wav");
}



int ao_is_big_endian(void) {
    int     endian = 1;

    if(*(char *)&endian) endian = 0;
    return(endian);
}


