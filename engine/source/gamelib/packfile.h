/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef SPK_SUPPORTED

#ifndef PACKFILE_H
#define PACKFILE_H

#include <stdio.h>

#include "globals.h"

#ifndef WIN
#include <unistd.h>
#define O_BINARY 0
#endif

#ifdef SDL
#include <SDL.h>
#endif

#define PACKFILE_PATH_MAX 512 // Maximum length of file path string.
#define MAX_TRACKS 256 // Maximum number of BGM Tracks. IMPORTANT: Wii max number is 256 (maybe for low memory??)

//
// Structure used for handling packfiles
//
typedef struct pnamestruct
{
    unsigned int pns_len;	    // Length of the struct in bytes
    unsigned int filestart;	    // Start position of referenced file
    unsigned int filesize;	    // Size of referenced file
    char		 namebuf[MAX_FILENAME_LEN];	// Buffer to hold the file's name
} pnamestruct;

typedef struct fileliststruct
{
    char filename[MAX_FILENAME_LEN];
    int nTracks;
    char bgmFileName[MAX_TRACKS][MAX_FILENAME_LEN];
    int bgmTrack;
    unsigned int bgmTracks[MAX_TRACKS];
#ifdef SDL
    SDL_Surface *preview;
#endif
} fileliststruct;

#define	NUMPACKHANDLES	8
#define PACKVERSION	0x00000000
#define testpackfile(filename, packfilename) closepackfile(openpackfile(filename, packfilename))

extern int printFileUsageStatistics;

// All of these return -1 on error
int openpackfile(const char *filename, const char *packfilename);
int readpackfile(int handle, void *buf, int len);
int closepackfile(int handle);
int seekpackfile(int handle, int offset, int whence);
int pak_init();
void pak_term();
void packfile_mode(int mode);
int pakopen(const char *filename, int mode);
int pakread(int fd, void *buf, int len);
void pakclose(int fd);
int paklseek(int fd, int n, int whence);
int openreadaheadpackfile(const char *filename, const char *packfilename, int readaheadsize, int prebuffersize);
int readpackfile_noblock(int fd, void *buf, int len);
int packfileeof(int fd);
int packfile_supported(const char *filename);
void packfile_music_read(struct fileliststruct *filelist, int dListTotal);
int packfile_music_play(struct fileliststruct *filelist, FILE *bgmFile, int bgmLoop, int curPos, int scrPos);
void freefilenamecache(void);

#endif

#endif
