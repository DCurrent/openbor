/*
 * OpenWav2Bor 1.1 - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under a BSD-style license, see LICENSE file for details.
 *
 * Copyright (c)  OpenBOR Team
 * Copyright (c) 2010 - 2011 Bryan Cain
 * 
 * This program was written by Bryan Cain (Plombo), but it reuses some code from 
 * OpenBOR.  It is a replacement for the closed-source Wav2Bor, which was 
 * originally written by Roel van Mastbergen of Senile Team, and updated to v1.1 
 * with stereo support by SX.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "adpcm.h"
#include "common.h"

/**
 * Reads a WAV file from the filesystem, storing its PCM stream and stream 
 * metadata.
 * @param filename path to the input WAV file
 * @param buf structure where the PCM stream and its metadata are stored
 * @return true on success, false on error
 */
bool loadwave(char *filename, samplestruct *buf)
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

	FILE *fd;

	fd = fopen(filename, "rb");
	if(fd == NULL) ioerror(strerror(errno));
	
	if(fread(&riffheader, sizeof(riffheader), 1, fd) != 1) ioerror("can't read from file");

	riffheader.riff = SwapLSB32(riffheader.riff);
	riffheader.size = SwapLSB32(riffheader.size);
	riffheader.type = SwapLSB32(riffheader.type);

	if(riffheader.riff!=HEX_RIFF || riffheader.type!=HEX_WAVE) ioerror("not a WAV file");

	rifftag.tag = 0;
	// Search for format tag
	while(rifftag.tag!=HEX_fmt)
	{
		if(fread(&rifftag, sizeof(rifftag), 1, fd)!=1) ioerror(strerror(errno));
		rifftag.tag = SwapLSB32(rifftag.tag);
		rifftag.size = SwapLSB32(rifftag.size);
		if(rifftag.tag!=HEX_fmt) fseek(fd,rifftag.size,SEEK_CUR);
	}
	if(fread(&fmt, sizeof(fmt), 1, fd)!=1) ioerror(strerror(errno));

	fmt.format = SwapLSB16(fmt.format);
	fmt.channels = SwapLSB16(fmt.channels);
	fmt.blockalign = SwapLSB16(fmt.blockalign);
	fmt.samplebits = SwapLSB16(fmt.samplebits);
	fmt.samplerate = SwapLSB32(fmt.samplerate);
	fmt.bps = SwapLSB32(fmt.bps);

	if(rifftag.size>sizeof(fmt)) fseek(fd,rifftag.size-sizeof(fmt),SEEK_CUR);

	if(fmt.format!=FMT_PCM || (fmt.channels!=1 && fmt.channels!=2) || fmt.samplebits!=16)
		ioerror("only 16-bit mono or stereo PCM WAV files are supported");

	// Search for data tag
	while(rifftag.tag!=HEX_data)
	{
		if(fread(&rifftag, sizeof(rifftag), 1, fd)!=1) ioerror(strerror(errno));
		rifftag.tag = SwapLSB32(rifftag.tag);
		rifftag.size = SwapLSB32(rifftag.size);
		if(rifftag.tag!=HEX_data) fseek(fd,rifftag.size,SEEK_CUR);
	}
	
	buf->sampleptr = malloc(rifftag.size);
	
	if(fmt.samplebits==8) memset(buf->sampleptr, 0x80, rifftag.size+8);
	else memset(buf->sampleptr, 0, rifftag.size+8);

	if(fread(buf->sampleptr, rifftag.size, 1, fd) != 1)
	{
		if(buf->sampleptr != NULL)
		{
			free(buf->sampleptr);
			buf->sampleptr = NULL;
		}
		ioerror(strerror(errno));
	}

	buf->soundlen = rifftag.size;
	buf->bits = fmt.samplebits;
	buf->frequency = fmt.samplerate;
	buf->channels = fmt.channels;
	
	fclose(fd);
	return true;

error:
	if(buf->sampleptr) { free(buf->sampleptr); buf->sampleptr = NULL; }
	if(fd) fclose(fd);
	return false;
}

/**
 * Saves stream metadata and encoded ADPCM stream to the filesystem as a BOR 
 * music file.
 * @param filename path to the output BOR file
 * @param buf structure containing the PCM stream and its metadata
 * @param artist artist name
 * @param title title of the song
 * @return true on success, false on error
 */
bool writebor(char *filename, samplestruct *buf, char *artist, char *title)
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
	
	int i=0, len = buf->soundlen;
	short *pcm = (short*)buf->sampleptr;
	char *adpcmbuf = malloc(len/4), *adpcm = adpcmbuf;
	FILE *fd = fopen(filename, "wb");
	
	if(fd == NULL) ioerror(strerror(errno));
	
	// write header
	strcpy(borheader.identifier, BOR_IDENTIFIER);
	strcpy(borheader.artist, artist);
	strcpy(borheader.title, title);
	borheader.version = SwapLSB32(NEW_BOR_VERSION);
	borheader.frequency = SwapLSB32(buf->frequency);
	borheader.channels = SwapLSB32(buf->channels);
	borheader.datastart = SwapLSB32(sizeof(borheader));
	
	if(fwrite(&borheader, sizeof(borheader), 1, fd) != 1) ioerror(strerror(errno));
	i += sizeof(borheader);
	printf("\rConverting... %09d", i);
	
	// encode raw PCM from the WAV as IMA ADPCM
	while(len>2048)
	{
		if(adpcm_encode(pcm, adpcm, 2048, buf->channels) != 512) ioerror("can't encode data as ADPCM");
		pcm += 1024;
		if(fwrite(adpcm, 1, 512, fd) != 512) ioerror(strerror(errno));
		adpcm += 512;
		i += 512;
		printf("\rConverting... %09d", i);
		len -= 2048;
	}
	
	if(adpcm_encode(pcm, adpcm, len, buf->channels) != (len/4)) ioerror("can't encode data as ADPCM");
	i += len/4;
	if(fwrite(adpcm, 1, len/4, fd) != (len/4)) ioerror(strerror(errno));
	printf("\rConverting... %09d\n", i);
	
	fclose(fd);
	free(buf->sampleptr);
	free(adpcmbuf);
	return true;

error:
	if(fd) fclose(fd);
	free(buf->sampleptr);
	free(adpcmbuf);
	return false;
}

/**
 * Replaces underscores in a string with spaces, and trims the string to 63 
 * characters, the length limit for artist/title names in BOR music files.
 * @param name the string
 * @return the modified string
 */
char * fixname(char *name)
{
	int i;

	for(i=0; i<strlen(name) && i<63; i++)
		if (name[i] == '_') name[i] = ' ';
	
	return name;
}

int main(int argc, char *argv[])
{
	int n;
	samplestruct buf = {NULL, 0, 0, 0, 0};
	short *pcm;
	char *adpcm = malloc(buf.soundlen/4);
	char *wavfile, *borfile, *artist, *title;
	bool success = true;
	
	if(argc < 3 || argc > 6)
	{
		printf("OpenWav2Bor 1.1, compile date " __DATE__ "\n");
		printf("Written by Plombo\n\n");
		printf("The original Wav2Bor was written by Roel from Senile Team and updated to v1.1\n"
		       "with stereo support by SX.  Although OpenWav2Bor is a replica of the original\n"
		       "Wav2Bor program, it does not use any code from it.\n\n");
		printf("Usage: %s file.wav file.bor [artist] [title]\n", argv[0]);
		printf("Replace spaces in the artist name and title with underscores (_).\n\n");
		printf("Example:\n"
		       "%s cooltune.wav cooltune.bor John_Smith Cool_Tune\n\n", argv[0]);
		printf("Contact and support at LavaLit: www.LavaLit.com\n");
		success = false;
	}
	else
	{
		wavfile = argv[1];
		borfile = argv[2];
		artist = argc >= 4 ? fixname(argv[3]) : "";
		title = argc >= 5 ? fixname(argv[4]) : "";
	}
	
	if(success) success = loadwave(wavfile, &buf);
	if(success) success = writebor(borfile, &buf, artist, title);
	
	if(success) printf("Done.\n");

#if defined(_WIN32) || defined(_WIN64)
	/* If the conversion fails, allow Windows users to see what went wrong 
	 * without the console window closing immediately. */
	if(!success) system("pause");
#endif
	return !success;
}


