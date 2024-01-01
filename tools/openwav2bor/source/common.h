/*
 * OpenWav2Bor and OpenBor2Wav 1.1 - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under a BSD-style license, see LICENSE file for details.
 *
 * Copyright (c)  OpenBOR Team
 * Copyright (c) 2010 - 2011 Bryan Cain
 */

#define		HEX_RIFF        0x46464952
#define		HEX_WAVE        0x45564157
#define		HEX_fmt         0x20746D66
#define		HEX_data        0x61746164
#define		FMT_PCM         0x0001
#define		OLD_BOR_VERSION 0x00010000
#define     NEW_BOR_VERSION 0x00010001
#define     BOR_IDENTIFIER  "BOR music"

#if (defined(__powerpc__) || defined(__ppc__)) // big endian
#define SwapLSB16(X) Swap16(X)
#define SwapLSB32(X) Swap32(X)
#else // little endian
#define SwapLSB16(X) (X)
#define SwapLSB32(X) (X)
#endif

#define printerr(msg) fprintf(stderr, "\nError: %s\n", msg)
#define ioerror(X) do { printerr(X); goto error; } while(0)

typedef struct {
	void *		   sampleptr;
	int			   soundlen;	 // Length in bytes
	int            bits;		 // 8/16 bit
	int            frequency;    // 11025 * 1,2,4
	int            channels;
} samplestruct;

