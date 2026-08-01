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

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE 1
#endif

#include <stdint.h>
#include <stdio.h>

#include "globals.h"

#ifndef WIN
#include <unistd.h>
#define O_BINARY 0
#endif

#ifdef SDL
#include <SDL.h>
#endif

/*
 * Full logical path buffer for assets inside pack archives.
 * MAX_FILENAME_LEN is intentionally smaller and is used for short names,
 * display titles, and other filename-only fields. Pack table entries store
 * full archive paths, so they use the engine path buffer size instead.
 */
#define PACKFILE_PATH_MAX MAX_BUFFER_LEN

typedef enum {

	/*
	* Binary format ID stored in packfile header. 
	* Format is stored as an 8-bit identifier in the
	* pack header, with bits 0-3 reserved for the 
	* ASCII magic "PACK" and bits 4-7 reserved for 
	* a binary format ID.
	*
	* Legacy engines only support PAK32 and always
	* expect bytes 4-7 to be zero, so PAK32 constant
	* must always be zero. 
	*/

	PAK_FORMAT_PAK32 = 0U,
	PAK_FORMAT_PAK64 = 1U
} e_packfile_format;


/*
 * Offsets and sizes in pack archives may be wider than the engine's
 * public read length. Keep reads as int-sized chunks, but keep every
 * archive/file position in a 64-bit type.
 */
typedef uint64_t packfile_offset_t;
typedef uint64_t packfile_size_t;
typedef int64_t packfile_signed_offset_t;

/*
 * In-memory representation of one pack table entry. PAK32 archives store
 * data_offset and data_size as 32-bit values. PAK64 archives store those
 * fields as 64-bit values, but both formats are normalized into this struct.
 */
typedef struct packfile_entry
{
    uint32_t record_size;                 // Size of this table record, including the name bytes.
    packfile_offset_t data_offset;        // Absolute byte offset where file data begins in the pack.
    packfile_size_t data_size;            // File data length in bytes.
    char name[PACKFILE_PATH_MAX];          // Null-terminated logical asset path from the pack table.
} packfile_entry;

/* Older code used pnamestruct. Keep the typedef as a compatibility alias. */
typedef packfile_entry pnamestruct;

typedef struct fileliststruct
{
    char filename[MAX_FILENAME_LEN];

    /*
     * Music track metadata is discovered by scanning the pack table. These
     * arrays are allocated to the exact number of discovered tracks instead
     * of using a fixed global limit. Call packfile_music_free() before
     * releasing or reusing a populated file list.
     */
    int nTracks;
    int bgmTrack;
    char (*bgmFileName)[MAX_FILENAME_LEN];
    packfile_offset_t *bgmTracks;
#ifdef SDL
    SDL_Surface *preview;
#endif
} fileliststruct;

/* Packfile handles are dynamically allocated integer indexes with no fixed engine limit. */
#define testpackfile(filename, packfilename) closepackfile(openpackfile(filename, packfilename))

extern int printFileUsageStatistics;

// All of these return -1 on error unless otherwise noted.
int openpackfile(const char *filename, const char *packfilename);
int readpackfile(int handle, void *buf, int len);
int closepackfile(int handle);
int seekpackfile(int handle, int offset, int whence);
packfile_signed_offset_t seekpackfile64(int handle, packfile_signed_offset_t offset, int whence);
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
void packfile_music_free(struct fileliststruct *filelist, int dListTotal);
int packfile_music_play(struct fileliststruct *filelist, FILE *bgmFile, int bgmLoop, int curPos, int scrPos);
void freefilenamecache(void);

#endif // PACKFILE_H

#endif // SPK_SUPPORTED
