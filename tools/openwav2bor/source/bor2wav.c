/*
 * OpenBor2Wav 1.1 - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under a BSD-style license, see LICENSE file for details.
 *
 * Copyright (c)  OpenBOR Team
 * Copyright (c) 2010 - 2011 Bryan Cain
 * 
 * This program was written by Bryan Cain, but it reuses some code from the 
 * OpenBOR engine.  It reuses some code from OpenWav2Bor, but performs the 
 * reverse operation, converting BOR music files to WAV files.  The version is 
 * numbered 1.1 to match OpenWav2Bor, and also since I released a bor2wav 
 * program written in Python some time ago that could be considered version 1.0.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "adpcm.h"
#include "common.h"

#define ADPCM_CHUNK_SIZE 2048

/**
 * Decodes the ADPCM to PCM and saves it as a WAV file.
 * @param filename the path to the output WAV file
 * @param buf structure containing the ADPCM stream and its metadata
 * @return true on success, false on error
 */
bool writewav(char *filename, samplestruct *buf)
{
	struct {
		uint32_t riff;
		uint32_t size;
		uint32_t type;
	} riffheader;
	struct {
		uint32_t tag;
		uint32_t size;
	} rifftag;
	struct {
		uint16_t format;	 // 1 = PCM
		uint16_t channels;	 // Mono, stereo
		uint32_t samplerate; // 11025, 22050, 44100
		uint32_t bps;		 // Bytes/second
		uint16_t blockalign;
		uint16_t samplebits; // 8, 12, 16
	} fmt;
	
	int i=0, len = buf->soundlen;
	unsigned char *adpcm = buf->sampleptr;
	short *pcm = malloc(ADPCM_CHUNK_SIZE * 4);
	FILE *fd = fopen(filename, "wb");
	if(fd == NULL) ioerror(strerror(errno));
	
	// write RIFF header
	riffheader.riff = SwapLSB32(HEX_RIFF);
	riffheader.size = SwapLSB32(sizeof(riffheader) - 8 + 2*sizeof(rifftag) + sizeof(fmt) + 4*buf->soundlen);
	riffheader.type = SwapLSB32(HEX_WAVE);
	if(fwrite(&riffheader, sizeof(riffheader), 1, fd) != 1) ioerror("can't write to file");
	
	// write format chunk
	rifftag.tag = SwapLSB32(HEX_fmt);
	rifftag.size = SwapLSB32(sizeof(fmt));
	fmt.format = SwapLSB16(FMT_PCM);
	fmt.channels = SwapLSB16(buf->channels);
	fmt.samplerate = SwapLSB32(buf->frequency);
	fmt.bps = SwapLSB32(buf->frequency * buf->channels * 2);
	fmt.blockalign = SwapLSB16(buf->channels * 2);
	fmt.samplebits = SwapLSB16(16);
	if(fwrite(&rifftag, sizeof(rifftag), 1, fd) != 1) ioerror(strerror(errno));
	if(fwrite(&fmt, sizeof(fmt), 1, fd) != 1) ioerror(strerror(errno));
	
	// write data chunk header
	rifftag.tag = SwapLSB32(HEX_data);
	rifftag.size = SwapLSB32(buf->soundlen * 4);
	if(fwrite(&rifftag, sizeof(rifftag), 1, fd) != 1) ioerror(strerror(errno));
	
	// decode ADPCM stream to PCM and write to WAV file
	while(len)
	{
		int adpcmsize = len < ADPCM_CHUNK_SIZE ? len : ADPCM_CHUNK_SIZE;
		int pcmsize = adpcmsize * 4;
		if(adpcm_decode(adpcm, pcm, adpcmsize, buf->channels) != pcmsize) ioerror("can't decode ADPCM data");
		if(fwrite(pcm, 1, pcmsize, fd) != pcmsize) ioerror(strerror(errno));
		adpcm += adpcmsize;
		i += pcmsize;
		printf("\rConverting... %09d", i);
		len -= adpcmsize;
	}
	printf("\n");
	
	free(buf->sampleptr);
	buf->sampleptr = NULL;
	free(pcm);
	fclose(fd);
	return true;

error:
	free(buf->sampleptr);
	buf->sampleptr = NULL;
	if(pcm) free(pcm);
	if(fd) fclose(fd);
	return false;
}

/**
 * Reads the stream metadata and encoded ADPCM stream from a BOR music file on 
 * the filesystem.
 * @param filename the path to the BOR file
 * @param buf structure where the ADPCM stream and its metadata is stored
 * @return true on success, false on failure
 */
bool readbor(char *filename, samplestruct *buf)
{
	struct {
		char           identifier[16];
		char           artist[64];
		char           title[64];
		uint32_t       version;
		int32_t        frequency;
		int32_t        channels;
		int32_t        datastart;
	} borheader;
	
	int i=0, len;
	FILE *fd = fopen(filename, "rb");
	
	if(fd == NULL) ioerror(strerror(errno));
	
	// read header
	if(fread(&borheader, sizeof(borheader), 1, fd) != 1) ioerror(strerror(errno));
	borheader.version = SwapLSB32(borheader.version);
	borheader.frequency = SwapLSB32(borheader.frequency);
	borheader.channels = SwapLSB32(borheader.channels);
	borheader.datastart = SwapLSB32(borheader.datastart);
	//assert(ftell(fd) == borheader.datastart); // ordinarily true, but not required by the BOR format
	
	// check header validity
	if((strcmp(borheader.identifier, BOR_IDENTIFIER) != 0) ||
		(borheader.version != OLD_BOR_VERSION && borheader.version != NEW_BOR_VERSION) ||
		(borheader.channels != 1 && borheader.channels != 2) ||
		(borheader.version == OLD_BOR_VERSION && borheader.channels == 2) ||
		(borheader.datastart != SwapLSB32(sizeof(borheader))))
	{
		ioerror("invalid BOR header");
	}
	
	// print artist and title
	if(*borheader.artist) printf("Artist: %s\n", borheader.artist);
	if(*borheader.title)  printf("Title: %s\n", borheader.title);
	if(*borheader.artist || *borheader.title) printf("\n"); // to make the output neater
	
	// get ADPCM stream size in bytes
	if (fseek(fd, 0, SEEK_END) != 0) ioerror(strerror(errno));
	len = ftell(fd) - borheader.datastart;
	
	// convert header info to stream info used for encoding
	buf->soundlen = len;
	fseek(fd, borheader.datastart, SEEK_SET);
	buf->bits = 16;
	buf->frequency = borheader.frequency;
	buf->channels = borheader.channels;
	
	// read ADPCM stream
	buf->sampleptr = malloc(len);
	if(buf->sampleptr == NULL) ioerror(strerror(errno));
	if(fseek(fd, borheader.datastart, SEEK_SET) != 0) ioerror(strerror(errno));
	if(fread(buf->sampleptr, 1, len, fd) != len) ioerror(strerror(errno));
	
	fclose(fd);
	return true;

error:
	if(buf->sampleptr) { free(buf->sampleptr); buf->sampleptr = NULL; }
	if(fd) fclose(fd);
	return false;
}

int main(int argc, char *argv[])
{
	samplestruct buf = {NULL, 0, 0, 0, 0};
	char *borfile, *wavfile;
	bool success = true;
	
	if(argc != 3)
	{
		printf("OpenWav2Bor 1.1, compile date " __DATE__ "\n");
		printf("Written by Plombo\n\n");
		printf("Usage: %s file.bor file.wav\n\n", argv[0]);
		printf("Example:\n"
		       "%s cooltune.bor cooltune.wav\n\n", argv[0]);
		printf("Contact and support at LavaLit: www.LavaLit.com\n");
		success = false;
	}
	else
	{
		borfile = argv[1];
		wavfile = argv[2];
	}
	
	if(success) success = readbor(borfile, &buf);
	if(success) success = writewav(wavfile, &buf);
	
	if(success) printf("Done.\n");

#if defined(_WIN32) || defined(_WIN64)
	/* If the conversion fails, allow Windows users to see what went wrong 
	 * without the console window closing immediately. */
	if(!success) system("pause");
#endif
	return !success;
}


