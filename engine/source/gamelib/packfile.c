/*
* OpenBOR - http://www.chronocrash.com
* -----------------------------------------------------------------------
* All rights reserved, see LICENSE in OpenBOR root for details.
*
* Copyright (c)  OpenBOR Team
*/

/*
* Code to read files from packfiles.
*
* Pack layout:
*     dword   magic      bytes 0-3, ASCII "PACK"
*     dword   format_id  bytes 4-7, binary numeric format id (see packfile_format enum)
*     bytes   file data
*     entries file table
*     footer  table_offset
*
* Each table entry stores record_size, data_offset, data_size, and name bytes
* including the trailing null. PAK32 uses 32-bit offsets, sizes, and table
* footer. PAK64 uses 64-bit offsets, sizes, and table footer.
*
* Archive positions and pack sizes are stored internally as 64-bit values.
* Individual readable assets are limited to INT_MAX bytes unless their format
* has a verified chunked reader using the 64-bit packfile API.
*/

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE 1
#endif

#include <assert.h>
#ifndef SPK_SUPPORTED

#include "openbor.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN
#include <strings.h>
#endif
#if WIN
#include <io.h>
#endif
#include "utils.h"
#include "borendian.h"
#include "stristr.h"
#include "packfile.h"
#include "packfile_handle.h"
#include "filecache.h"
#include "soundmix.h"
#include "savedata.h"
#include "List.h"

#if WIN || LINUX
#include <dirent.h>
#endif

#if _POSIX_SOURCE
#define stricmp strcasecmp
#endif

#pragma pack (1)

/*
* Requirements for packfiles.
*/
#define PACKMAGIC      0x4B434150U
#define PACK_HEADER_SIZE 8U
#define PAK32_TABLE_ENTRY_HEADER_SIZE 12U
#define PAK64_TABLE_ENTRY_HEADER_SIZE  20U
#define PAK32_FOOTER_SIZE 4U
#define PAK64_FOOTER_SIZE  8U
/*
* PAK32 packs use unsigned 32-bit offsets and sizes on disk. The engine accepts
* the full unsigned 32-bit range; the packer should use a lower production limit
* so table/footer growth cannot push a PAK32 archive past this boundary.
*/
#define PAK32_MAXIMUM_ARCHIVE_SIZE ((packfile_size_t)UINT32_MAX)
/*
* filecache.c still uses signed int-sized sector math. Packs above this size
* are valid only through the direct 64-bit reader.
*/
#define PACK_CACHE_MAXIMUM_SIZE ((packfile_size_t)(INT_MAX - 2047))
/*
* The archive format can store larger individual files, but the engine's
* public read functions still accept int byte counts and return int byte
* counts. Only formats with verified 64-bit streaming readers may exceed
* this limit.
*/
#define PACKFILE_MAXIMUM_ASSET_SIZE ((packfile_size_t)INT_MAX)
static const size_t USED_FLAG = (((size_t) 1) << ((sizeof(size_t) * 8) - 1));

/*
* These defines are only used for cached code.
* CACHEBLOCKSIZE * CACHEBLOCKS is the size of the ever-present file cache.
* Cache blocks must be 255 or less.
*/
#define CACHEBLOCKSIZE (32768)
#define CACHEBLOCKS    (96)

static int pak_initialized;
static int pak_cache_enabled = 1;
int printFileUsageStatistics = 0;

/*
* Direct and cached readers share dynamically allocated integer handles.
* Handle storage grows as needed and released slots are reused in constant
* time. The table itself remains allocated until pak_term().
*/
static s_packfile_handle_table packfile_handle_table = { NULL, 0, -1 };

/*
* char packfile[128] is defined in sdl/sdlport.c.
*/
List *filenamelist = NULL;

/*
* These variables are only used for cached code.
*/
static int pakfd = -1;
static size_t pak_entry_header_size = PAK32_TABLE_ENTRY_HEADER_SIZE;
static size_t pak_footer_size = PAK32_FOOTER_SIZE;
static packfile_size_t paksize;
static packfile_offset_t pak_headerstart;
static size_t pak_headersize;
static unsigned char *pak_cdheader;
static unsigned char *pak_header;

typedef struct packfile_format {
    uint32_t format_id;
    size_t entry_header_size;
    size_t footer_size;
    packfile_size_t file_size;
    packfile_offset_t table_offset;
    packfile_offset_t table_end_offset;
} packfile_format;

/*
* Pointers to the real functions.
*/
typedef int (*OpenPackfile)(const char *, const char *);

int openPackfile(const char *, const char *);
int readPackfile(int, void *, int);
packfile_signed_offset_t seekPackfile64(int, packfile_signed_offset_t, int);
int seekPackfile(int, int, int);
int closePackfile(int);
int openPackfileCached(const char *, const char *);
int readPackfileCached(int, void *, int);
packfile_signed_offset_t seekPackfileCached64(int, packfile_signed_offset_t, int);
int seekPackfileCached(int, int, int);
int closePackfileCached(int);

static OpenPackfile pOpenPackfile = openPackfile;

/*
* Portable 64-bit seek and read helpers.
*/
static packfile_signed_offset_t packfile_seek_fd(int file_descriptor, packfile_signed_offset_t offset, int whence) {
#if WIN
    return (packfile_signed_offset_t)_lseeki64(file_descriptor, offset, whence);
#else
    off_t result;

    result = lseek(file_descriptor, (off_t)offset, whence);
    if(result == (off_t)-1)  {
        return -1;
    }
    return (packfile_signed_offset_t)result;
#endif
}

static int packfile_seek_fd_unsigned(int file_descriptor, packfile_offset_t offset, int whence) {
    if(offset > (packfile_offset_t)INT64_MAX)  {
        return -1;
    }
    return packfile_seek_fd(file_descriptor, (packfile_signed_offset_t)offset, whence) < 0 ? -1 : 0;
}

static int packfile_read_exact_fd(int file_descriptor, void *buffer, size_t bytes_to_read) {
    unsigned char *write_position = (unsigned char *)buffer;

    while(bytes_to_read > 0)  {
        int bytes_read;
        size_t chunk_size = bytes_to_read;

        if(chunk_size > (size_t)INT_MAX)  {
            chunk_size = (size_t)INT_MAX;
        }

        bytes_read = (int)read(file_descriptor, write_position, (unsigned int)chunk_size);
        if(bytes_read <= 0)  {
            return 0;
        }

        write_position += bytes_read;
        bytes_to_read -= (size_t)bytes_read;
    }

    return 1;
}

static uint32_t packfile_read_lsb32_from_memory(const unsigned char *source) {
    return ((uint32_t)source[0]) |
           ((uint32_t)source[1] << 8) |
           ((uint32_t)source[2] << 16) |
           ((uint32_t)source[3] << 24);
}

static uint64_t packfile_read_lsb64_from_memory(const unsigned char *source) {
    return ((uint64_t)source[0]) |
           ((uint64_t)source[1] << 8) |
           ((uint64_t)source[2] << 16) |
           ((uint64_t)source[3] << 24) |
           ((uint64_t)source[4] << 32) |
           ((uint64_t)source[5] << 40) |
           ((uint64_t)source[6] << 48) |
           ((uint64_t)source[7] << 56);
}

static int packfile_read_lsb32_fd(int file_descriptor, uint32_t *value) {
    unsigned char bytes[4];

    if(!packfile_read_exact_fd(file_descriptor, bytes, sizeof(bytes)))  {
        return 0;
    }
    *value = packfile_read_lsb32_from_memory(bytes);
    return 1;
}

static int packfile_read_lsb64_fd(int file_descriptor, uint64_t *value) {
    unsigned char bytes[8];

    if(!packfile_read_exact_fd(file_descriptor, bytes, sizeof(bytes)))  {
        return 0;
    }
    *value = packfile_read_lsb64_from_memory(bytes);
    return 1;
}

static size_t packfile_entry_header_size_for_footer(size_t footer_size) {
    return footer_size == PAK64_FOOTER_SIZE ? PAK64_TABLE_ENTRY_HEADER_SIZE : PAK32_TABLE_ENTRY_HEADER_SIZE;
}

static int packfile_read_footer_candidate(int file_descriptor, packfile_size_t file_size, size_t footer_size, packfile_offset_t *table_offset) {
    uint32_t pak32_offset;
    uint64_t pak64_offset;

    /*
    * A valid pack begins with the 8-byte magic/format_id header and ends with
    * either a 32-bit PAK32 footer or a 64-bit PAK64 footer.
    */
    if(file_size < (packfile_size_t)(PACK_HEADER_SIZE + footer_size))  {
        return 0;
    }

    if(packfile_seek_fd(file_descriptor, -(packfile_signed_offset_t)footer_size, SEEK_END) < 0)  {
        return 0;
    }

    if(footer_size == PAK64_FOOTER_SIZE)  {
        if(!packfile_read_lsb64_fd(file_descriptor, &pak64_offset))  {
            return 0;
        }
        *table_offset = (packfile_offset_t)pak64_offset;
    }
    else  {
        if(!packfile_read_lsb32_fd(file_descriptor, &pak32_offset))  {
            return 0;
        }
        *table_offset = (packfile_offset_t)pak32_offset;
    }

    return 1;
}

static int packfile_validate_table_candidate(int file_descriptor, packfile_size_t file_size, size_t footer_size, packfile_offset_t table_offset) {
    uint32_t record_size;
    size_t entry_header_size = packfile_entry_header_size_for_footer(footer_size);
    packfile_offset_t table_end_offset;

    if(file_size < footer_size)  {
        return 0;
    }

    table_end_offset = file_size - footer_size;
    if(table_offset < PACK_HEADER_SIZE || table_offset > table_end_offset)  {
        return 0;
    }

    if(table_offset == table_end_offset)  {
        return 1;
    }

    if(table_end_offset - table_offset < entry_header_size)  {
        return 0;
    }

    if(packfile_seek_fd_unsigned(file_descriptor, table_offset, SEEK_SET) < 0)  {
        return 0;
    }

    if(!packfile_read_lsb32_fd(file_descriptor, &record_size))  {
        return 0;
    }

    if(record_size <= entry_header_size)  {
        return 0;
    }

    if((packfile_offset_t)record_size > table_end_offset - table_offset)  {
        return 0;
    }

    return 1;
}

static int packfile_detect_table_format(int file_descriptor, packfile_size_t file_size, uint32_t format_id, packfile_format *format) {
    packfile_offset_t table_offset;
    size_t footer_size;

    if(format_id == PAK_FORMAT_PAK64)  {
        footer_size = PAK64_FOOTER_SIZE;
    }
    else if(format_id == PAK_FORMAT_PAK32)  {
        /*
        * PAK32 packs are only accepted when the final archive fits the
        * unsigned 32-bit on-disk offset model. The packer may enforce a lower
        * data-size limit to leave room for the table and footer, but the engine
        * should accept any completed PAK32 pack that remains in range.
        */
        if(file_size > PAK32_MAXIMUM_ARCHIVE_SIZE)  {
            return 0;
        }
        footer_size = PAK32_FOOTER_SIZE;
    }
    else  {
        return 0;
    }

    if(!packfile_read_footer_candidate(file_descriptor, file_size, footer_size, &table_offset) ||
       !packfile_validate_table_candidate(file_descriptor, file_size, footer_size, table_offset))  {
        return 0;
    }

    format->format_id = format_id;
    format->footer_size = footer_size;
    format->entry_header_size = packfile_entry_header_size_for_footer(footer_size);
    format->file_size = file_size;
    format->table_offset = table_offset;
    format->table_end_offset = file_size - footer_size;
    return 1;
}

static int packfile_read_format_fd(int file_descriptor, packfile_format *format) {
    uint32_t magic;
    uint32_t format_id;
    packfile_signed_offset_t file_size;

    if(packfile_seek_fd(file_descriptor, 0, SEEK_SET) < 0)  {
        return 0;
    }

    if(!packfile_read_lsb32_fd(file_descriptor, &magic) || magic != PACKMAGIC)  {
        return 0;
    }

    if(!packfile_read_lsb32_fd(file_descriptor, &format_id))  {
        return 0;
    }

    file_size = packfile_seek_fd(file_descriptor, 0, SEEK_END);
    if(file_size < 0)  {
        return 0;
    }

    return packfile_detect_table_format(file_descriptor, (packfile_size_t)file_size, format_id, format);
}

static int packfile_read_entry_fd(int file_descriptor, const packfile_format *format, packfile_offset_t table_position, packfile_entry *entry) {
    uint32_t record_size;
    uint32_t pak32_offset;
    uint32_t pak32_size;
    uint64_t pak64_offset;
    uint64_t pak64_size;
    packfile_size_t name_bytes;
    size_t bytes_to_copy;

    memset(entry, 0, sizeof(*entry));

    if(table_position >= format->table_end_offset)  {
        return 0;
    }

    if((format->table_end_offset - table_position) < format->entry_header_size)  {
        return 0;
    }

    if(packfile_seek_fd_unsigned(file_descriptor, table_position, SEEK_SET) < 0)  {
        return 0;
    }

    if(!packfile_read_lsb32_fd(file_descriptor, &record_size))  {
        return 0;
    }

    if(record_size <= format->entry_header_size || (packfile_offset_t)record_size > format->table_end_offset - table_position)  {
        return 0;
    }

    if(format->entry_header_size == PAK64_TABLE_ENTRY_HEADER_SIZE)  {
        if(!packfile_read_lsb64_fd(file_descriptor, &pak64_offset) || !packfile_read_lsb64_fd(file_descriptor, &pak64_size))  {
            return 0;
        }
        entry->data_offset = (packfile_offset_t)pak64_offset;
        entry->data_size = (packfile_size_t)pak64_size;
    }
    else  {
        if(!packfile_read_lsb32_fd(file_descriptor, &pak32_offset) || !packfile_read_lsb32_fd(file_descriptor, &pak32_size))  {
            return 0;
        }
        entry->data_offset = (packfile_offset_t)pak32_offset;
        entry->data_size = (packfile_size_t)pak32_size;
    }

    entry->record_size = record_size;
    name_bytes = (packfile_size_t)record_size - format->entry_header_size;
    bytes_to_copy = name_bytes < (sizeof(entry->name) - 1) ? (size_t)name_bytes : (sizeof(entry->name) - 1);

    if(bytes_to_copy > 0 && !packfile_read_exact_fd(file_descriptor, entry->name, bytes_to_copy))  {
        return 0;
    }
    entry->name[bytes_to_copy] = '\0';

    return 1;
}

static int packfile_data_range_is_valid(packfile_offset_t data_offset, packfile_size_t data_size, packfile_offset_t table_offset) {
    if(data_offset < PACK_HEADER_SIZE)  {
        return 0;
    }

    if(data_offset > table_offset)  {
        return 0;
    }

    if(data_size > table_offset - data_offset)  {
        return 0;
    }

    return 1;
}

static int packfile_entry_data_range_is_valid(const packfile_format *format, const packfile_entry *entry) {
    return packfile_data_range_is_valid(entry->data_offset, entry->data_size, format->table_offset);
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Identify formats whose readers retain positions
* wider than int and consume data in bounded chunks.
* WAV and WebM meet this requirement on every
* supported target. Vorbisfile exposes tell_func as
* long, so OGG/OGA qualify only on LP64 targets.
*/
static int packfile_asset_size_limit_exempt(const char *asset_name) {
    const char *extension;

    if(!asset_name) {
        return 0;
    }

    extension = strrchr(asset_name, '.');
    if(!extension) {
        return 0;
    }

    if(!stricmp(extension, ".wav") ||
       !stricmp(extension, ".webm")) {
        return 1;
    }

#if LONG_MAX > INT_MAX
    if(!stricmp(extension, ".ogg") ||
       !stricmp(extension, ".oga")) {
        return 1;
    }
#endif

    return 0;
}

static int packfile_asset_size_is_supported(packfile_size_t asset_size, const char *asset_name) {
    if(asset_size <= PACKFILE_MAXIMUM_ASSET_SIZE || packfile_asset_size_limit_exempt(asset_name))  {
        return 1;
    }

    printf("asset exceeds engine file size limit: %s (%" PRIu64 " bytes, max %" PRIu64 ")\n",
           asset_name ? asset_name : "(unknown)",
           (uint64_t)asset_size,
           (uint64_t)PACKFILE_MAXIMUM_ASSET_SIZE);
    return 0;
}

static size_t packfile_cached_entry_name_offset(void) {
    return pak_entry_header_size;
}

static uint32_t packfile_cached_record_size(size_t header_offset) {
    return packfile_read_lsb32_from_memory(pak_header + header_offset);
}

static packfile_offset_t packfile_cached_data_offset(size_t header_offset) {
    if(pak_entry_header_size == PAK64_TABLE_ENTRY_HEADER_SIZE)  {
        return packfile_read_lsb64_from_memory(pak_header + header_offset + 4);
    }
    return packfile_read_lsb32_from_memory(pak_header + header_offset + 4);
}

static packfile_size_t packfile_cached_data_size(size_t header_offset) {
    if(pak_entry_header_size == PAK64_TABLE_ENTRY_HEADER_SIZE)  {
        return packfile_read_lsb64_from_memory(pak_header + header_offset + 12);
    }
    return packfile_read_lsb32_from_memory(pak_header + header_offset + 8);
}

static int packfile_cached_data_range_is_valid(packfile_offset_t data_offset, packfile_size_t data_size) {
    if(!packfile_data_range_is_valid(data_offset, data_size, pak_headerstart))  {
        return 0;
    }

    if(data_size > paksize - data_offset)  {
        return 0;
    }

    return 1;
}

static int packfile_offset_to_int(packfile_offset_t value, int *converted) {
    if(value > (packfile_offset_t)INT_MAX)  {
        return 0;
    }
    *converted = (int)value;
    return 1;
}

/*
* Normalize archive path characters for case-insensitive lookup.
* Backslashes are treated as forward slashes so paths from older Windows tools
* match engine lookups on all supported platforms.
*/
static char packfile_normalized_path_character(const char *source_character) {
    static const char uppercase_to_lowercase_delta = 'a' - 'A';
    switch(*source_character)  {
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
        return *source_character + uppercase_to_lowercase_delta;
    case '\\':
        return '/';
    default:
        return *source_character;
    }
    return '\0';
}

/*
* Normalize a path buffer in-place before inserting it into the filename cache.
*/
static void packfile_normalize_path_in_place(char *path) {
    char *current_character = path;
    while(current_character && *current_character)  {
        *current_character = packfile_normalized_path_character(current_character);
        current_character++;
    }
}

/*
* Return 0 when names match after packfile path normalization. Non-zero return
* values mean the names are different, matching the old comparison contract.
*/
static int packfile_compare_names_normalized(const char *first_name, size_t first_size, const char *second_name, size_t second_size) {
    const char *first_cursor;
    const char *second_cursor;

    if(first_name == second_name)  {
        return 0;
    }
    if(first_size != second_size)  {
        return 1;
    }

    first_cursor = first_name;
    second_cursor = second_name;

    for(;;)  {
        if(!*first_cursor)  {
            return *second_cursor ? -1 : 0;
        }
        if(!*second_cursor)  {
            return 1;
        }
        if((*first_cursor != *second_cursor) && (packfile_normalized_path_character(first_cursor) != packfile_normalized_path_character(second_cursor)))  {
            return 1;
        }

        first_cursor++;
        second_cursor++;
    }
    return -2;
}

#if WIN
/*
* Convert slashes (UNIX) to backslashes (DOS).
* Return a pointer to buffer with filename converted to DOS format.
*/
static char *slashback(const char *source) {
    int index = 0;
    static char new_path[PACKFILE_PATH_MAX];
    while(source[index] && index < PACKFILE_PATH_MAX - 1)  {
        new_path[index] = source[index];
        if(new_path[index] == '/')  {
            new_path[index] = '\\';
        }
        ++index;
    }
    new_path[index] = 0;
    return new_path;
}
#endif

#ifndef WIN
/*
* Convert backslashes (DOS) to forward slashes (everything else).
* Return a pointer to buffer with filename using forward slash as separator.
*/
static char *slashfwd(const char *source) {
    int index = 0;
    static char new_path[PACKFILE_PATH_MAX];
    while(source[index] && index < PACKFILE_PATH_MAX - 1)  {
        new_path[index] = source[index];
        if(new_path[index] == '\\')  {
            new_path[index] = '/';
        }
        ++index;
    }
    new_path[index] = 0;
    return new_path;
}
#endif

#ifdef LINUX
char *casesearch(const char *dir, const char *filepath) {
    DIR *directory;
    struct dirent *entry;
    char filename[PACKFILE_PATH_MAX] = {""};
    const char *rest_of_path;
    static char fullpath[PACKFILE_PATH_MAX];
    int found = 0;
#ifdef VERBOSE
    printf("casesearch: %s, %s\n", dir, filepath);
#endif

    if((directory = opendir(dir)) == NULL)  {
        return NULL;
    }

    rest_of_path = strchr(filepath, '/');
    if(rest_of_path != NULL)  {
        if(rest_of_path - filepath <= 0)  {
            closedir(directory);
            return NULL;
        }
        strncat(filename, filepath, rest_of_path - filepath);
        rest_of_path++;
    }
    else  {
        strcpy(filename, filepath);
    }

    while((entry = readdir(directory)) != NULL)  {
        if(stricmp(entry->d_name, filename) == 0)  {
            found = 1;
            break;
        }
    }

    if(entry != NULL)  {
        strcpy(fullpath, dir);
        strcat(fullpath, "/");
        strcat(fullpath, entry->d_name);
    }

    if(closedir(directory))  {
        return NULL;
    }
    if(!found)  {
        return NULL;
    }

    return rest_of_path == NULL ? fullpath : casesearch(fullpath, rest_of_path);
}
#endif


static int packfile_acquire_handle(e_packfile_handle_type type) {
    int handle;

    handle = packfile_handle_acquire(&packfile_handle_table, type);
    if(handle < 0)  {
        printf("unable to allocate packfile handle\n");
    }
    return handle;
}

static void packfile_release_all_handles(void) {
    size_t handle_index;

    for(handle_index = 0; handle_index < packfile_handle_table.capacity; handle_index++)  {
        s_packfile_handle *handle = packfile_handle_get(&packfile_handle_table, (int)handle_index);

        if(!handle)  {
            continue;
        }
        if(handle->type == PACKFILE_HANDLE_DIRECT && handle->file_descriptor >= 0)  {
            close(handle->file_descriptor);
        }
        else if(handle->type == PACKFILE_HANDLE_CACHED)  {
            filecache_setvfd((int)handle_index, -1, -1, 0);
        }
    }

    packfile_handle_table_destroy(&packfile_handle_table);
}

void packfile_mode(int mode) {
    if(!mode || !pak_cache_enabled)  {
        pOpenPackfile = openPackfile;
        return;
    }

    pOpenPackfile = openPackfileCached;
}


#if WIN
int isRawData() {
    DIR *directory;
    if((directory = opendir("data")) == NULL)  {
        return 0;
    }
    closedir(directory);
    return 1;
}
#endif

#if LINUX
int isRawData() {
    DIR *directory;
    struct dirent *entry;
    directory = opendir(".");

    if(directory != NULL)  {
        while((entry = readdir(directory)))  {
            if(strcasecmp("data", entry->d_name) == 0)  {
                (void)closedir(directory);
                return 1;
            }
        }
    }
    if(directory != NULL)  {
        (void)closedir(directory);
    }
    return 0;
}
#endif


/*
- Caskey, Damon V.
- 2026-08-19
-
- Open a loose filesystem asset through the direct
  handle table. This preserves external file overrides
  while allowing packed assets to remain cached.
*/
static int openPackfileLoose(const char *filename) {
    int virtual_handle;
    int real_handle;
    int file_permission = 666;
    packfile_signed_offset_t loose_file_size;
    s_packfile_handle *handle_record;
    const char *disk_filename;
#ifdef LINUX
    char *case_corrected_path;
#endif

    if(!filename)  {
        return -1;
    }

#ifdef WIN
    disk_filename = slashback(filename);
#else
    disk_filename = slashfwd(filename);
#endif

    real_handle = open(disk_filename, O_RDONLY | O_BINARY, file_permission);

#ifdef LINUX
    if(real_handle == -1)  {
        case_corrected_path = casesearch(".", disk_filename);
        if(case_corrected_path != NULL)  {
            disk_filename = case_corrected_path;
            real_handle = open(disk_filename, O_RDONLY | O_BINARY, file_permission);
        }
    }
#endif

    if(real_handle == -1)  {
        return -1;
    }

    loose_file_size = packfile_seek_fd(real_handle, 0, SEEK_END);
    if(loose_file_size < 0 ||
       packfile_seek_fd(real_handle, 0, SEEK_SET) < 0 ||
       !packfile_asset_size_is_supported((packfile_size_t)loose_file_size, disk_filename))  {
        close(real_handle);
        return -1;
    }

    virtual_handle = packfile_acquire_handle(PACKFILE_HANDLE_DIRECT);
    if(virtual_handle == -1)  {
        close(real_handle);
        return -1;
    }

    handle_record = packfile_handle_get_type(
        &packfile_handle_table,
        virtual_handle,
        PACKFILE_HANDLE_DIRECT
    );
    handle_record->file_descriptor = real_handle;
    handle_record->size = (packfile_size_t)loose_file_size;
    return virtual_handle;
}

int openpackfile(const char *filename, const char *packfilename) {
    int loose_handle;

#ifdef VERBOSE
    char *pointsto;

    if(pOpenPackfile == openPackfileCached)  {
        pointsto = "openPackfileCached";
    }
    else if(pOpenPackfile == openPackfile)  {
        pointsto = "openPackfile";
    }
    else  {
        pointsto = "unknown destination";
    }
    printf("openpackfile called: f: %s, p: %s, dest: %s\n", filename, packfilename, pointsto);
#endif

    if(pOpenPackfile == openPackfileCached)  {
        loose_handle = openPackfileLoose(filename);
        if(loose_handle >= 0)  {
            return loose_handle;
        }
    }

    return pOpenPackfile(filename, packfilename);
}

int openPackfile(const char *filename, const char *packfilename) {
    int virtual_handle;
    int real_handle;
    int file_permission = 666;
    s_packfile_handle *handle_record;
    const char *pak_filename;
    packfile_format format;
    packfile_entry entry;
    packfile_offset_t table_position;

    virtual_handle = openPackfileLoose(filename);
    if(virtual_handle >= 0)  {
        return virtual_handle;
    }

    virtual_handle = packfile_acquire_handle(PACKFILE_HANDLE_DIRECT);
    if(virtual_handle == -1)  {
        return -1;
    }
    handle_record = packfile_handle_get_type(&packfile_handle_table, virtual_handle, PACKFILE_HANDLE_DIRECT);
    pak_filename = filename;

    real_handle = open(packfilename, O_RDONLY | O_BINARY, file_permission);
    if(real_handle == -1)  {
        packfile_handle_release(&packfile_handle_table, virtual_handle);
        return -1;
    }

    if(!packfile_read_format_fd(real_handle, &format))  {
        close(real_handle);
        packfile_handle_release(&packfile_handle_table, virtual_handle);
        return -1;
    }

    table_position = format.table_offset;
    while(table_position < format.table_end_offset)  {
        if(!packfile_read_entry_fd(real_handle, &format, table_position, &entry))  {
            break;
        }

        if(packfile_compare_names_normalized(pak_filename, strlen(pak_filename), entry.name, strlen(entry.name)) == 0)  {
            if(!packfile_entry_data_range_is_valid(&format, &entry) ||
               !packfile_asset_size_is_supported(entry.data_size, entry.name))  {
                close(real_handle);
                packfile_handle_release(&packfile_handle_table, virtual_handle);
                return -1;
            }

            if(packfile_seek_fd_unsigned(real_handle, entry.data_offset, SEEK_SET) < 0)  {
                close(real_handle);
                packfile_handle_release(&packfile_handle_table, virtual_handle);
                return -1;
            }

            handle_record->file_descriptor = real_handle;
            handle_record->size = entry.data_size;
            return virtual_handle;
        }

        table_position += entry.record_size;
    }

    close(real_handle);
    packfile_handle_release(&packfile_handle_table, virtual_handle);
    return -1;
}

static int update_filecache_vfd(int virtual_file_descriptor) {
    int start_block;
    int read_block;
    int readahead_blocks;
    s_packfile_handle *handle_record;

    handle_record = packfile_handle_get_type(&packfile_handle_table, virtual_file_descriptor, PACKFILE_HANDLE_CACHED);
    if(!handle_record)  {
        return filecache_setvfd(virtual_file_descriptor, -1, -1, 0);
    }

    if(!packfile_offset_to_int(handle_record->data_start / CACHEBLOCKSIZE, &start_block) ||
       !packfile_offset_to_int((handle_record->data_start + handle_record->position) / CACHEBLOCKSIZE, &read_block))  {
        return filecache_setvfd(virtual_file_descriptor, -1, -1, 0);
    }

    readahead_blocks = (handle_record->readahead_size + (CACHEBLOCKSIZE - 1)) / CACHEBLOCKSIZE;
    return filecache_setvfd(virtual_file_descriptor, start_block, read_block, readahead_blocks);
}

void makefilenamecache(void) {
    size_t header_position;
    size_t name_offset;
    uint32_t record_size;
    char target[PACKFILE_PATH_MAX];

    if(!filenamelist)  {
        filenamelist = malloc(sizeof(List));
    }
    List_Init(filenamelist);

    name_offset = packfile_cached_entry_name_offset();
    header_position = 0;
    for(;;)  {
        if((header_position + pak_entry_header_size) >= pak_headersize)  {
            return;
        }

        record_size = packfile_cached_record_size(header_position);
        if(record_size <= pak_entry_header_size || (header_position + record_size) > pak_headersize)  {
            return;
        }

        if((header_position & USED_FLAG) == USED_FLAG)  {
            printf("pack header is too large for filename usage tracking\n");
            return;
        }

        strncpy(target, (char *)pak_header + header_position + name_offset, PACKFILE_PATH_MAX - 1);
        target[PACKFILE_PATH_MAX - 1] = '\0';
        packfile_normalize_path_in_place(target);
        List_InsertAfter(filenamelist, (void *)header_position, target);
        header_position += record_size;
    }
}

void freefilenamecache(void) {
    Node *node;
    size_t count = 0;
    size_t total = 0;
    if(filenamelist)  {
        if(printFileUsageStatistics)  {
            printf("unused files in the pack:\n");
            List_GotoFirst(filenamelist);
            node = List_GetCurrentNode(filenamelist);
            while(node)  {
                total++;
                if(((size_t)node->value & USED_FLAG) != USED_FLAG)  {
                    count++;
                    printf("%s\n", node->name);
                }
                if(List_GotoNext(filenamelist))  {
                    node = List_GetCurrentNode(filenamelist);
                }
                else  {
                    break;
                }
            }
            printf("Summary: %d of %d unused files.\n", (int)count, (int)total);
            printf("WARNING\n");
            printf("to be completely sure if a file is unused, you have to play the entire game.\n");
            printf("in every possible branch, including every possible player, and so forth.\n");
            printf("so only remove stuff from a foreign project if you're completely sure that it is unused.\n");
        }
        List_Clear(filenamelist);
        free(filenamelist);
        filenamelist = NULL;
    }
}

int openreadaheadpackfile(const char *filename, const char *packfilename, int readaheadsize, int prebuffersize) {
    size_t header_position;
    int virtual_file_descriptor;
    size_t packfile_name_length;
    size_t requested_packfile_name_length;
    packfile_offset_t data_offset;
    packfile_size_t data_size;
    char target[PACKFILE_PATH_MAX];
    Node *node;
    s_packfile_handle *handle_record;

    if(!pak_cache_enabled)  {
        return openPackfile(filename, packfilename);
    }

    if(packfilename != packfile)  {
        packfile_name_length = strlen(packfile);
        requested_packfile_name_length = strlen(packfilename);
        if(packfile_compare_names_normalized(packfilename, requested_packfile_name_length, packfile, packfile_name_length))  {
            printf("tried to open from unknown pack file (%s)\n", packfilename);
            return -1;
        }
    }

    if(!filenamelist)  {
        makefilenamecache();
    }

    strncpy(target, filename, PACKFILE_PATH_MAX - 1);
    target[PACKFILE_PATH_MAX - 1] = '\0';
    packfile_normalize_path_in_place(target);

    node = List_GetNodeByName(filenamelist, target);
    if(!node)  {
        return -1;
    }

    header_position = (size_t)node->value & ~USED_FLAG;
    node->value = (void *)(((size_t)node->value) | USED_FLAG);

    data_offset = packfile_cached_data_offset(header_position);
    data_size = packfile_cached_data_size(header_position);
    if(!packfile_cached_data_range_is_valid(data_offset, data_size) ||
       !packfile_asset_size_is_supported(data_size, target))  {
        return -1;
    }

    virtual_file_descriptor = packfile_acquire_handle(PACKFILE_HANDLE_CACHED);
    if(virtual_file_descriptor < 0)  {
        return -1;
    }

    handle_record = packfile_handle_get_type(&packfile_handle_table, virtual_file_descriptor, PACKFILE_HANDLE_CACHED);
    handle_record->data_start = data_offset;
    handle_record->size = data_size;
    handle_record->readahead_size = readaheadsize;

    if(!update_filecache_vfd(virtual_file_descriptor))  {
        packfile_handle_release(&packfile_handle_table, virtual_file_descriptor);
        return -1;
    }

    if(prebuffersize > 0)  {
        filecache_wait_for_prebuffer(virtual_file_descriptor, (prebuffersize + (CACHEBLOCKSIZE - 1)) / CACHEBLOCKSIZE);
    }
    return virtual_file_descriptor;
}

int openPackfileCached(const char *filename, const char *packfilename) {
    return openreadaheadpackfile(filename, packfilename, 0, 0);
}


int readpackfile(int handle, void *buf, int len) {
    s_packfile_handle *handle_record = packfile_handle_get(&packfile_handle_table, handle);

    if(!handle_record)  {
        return -1;
    }
    if(handle_record->type == PACKFILE_HANDLE_DIRECT)  {
        return readPackfile(handle, buf, len);
    }
    return readPackfileCached(handle, buf, len);
}

int readPackfile(int handle, void *buf, int len) {
    packfile_size_t bytes_remaining;
    int bytes_read;
    s_packfile_handle *handle_record;

    if(len < 0)  {
        return -1;
    }
    if(len == 0)  {
        return 0;
    }

    handle_record = packfile_handle_get_type(&packfile_handle_table, handle, PACKFILE_HANDLE_DIRECT);
    if(!handle_record || handle_record->file_descriptor < 0)  {
        return -1;
    }

    if(handle_record->position >= handle_record->size)  {
        return 0;
    }

    bytes_remaining = handle_record->size - handle_record->position;
    if((packfile_size_t)len > bytes_remaining)  {
        len = bytes_remaining > (packfile_size_t)INT_MAX ? INT_MAX : (int)bytes_remaining;
    }

    bytes_read = (int)read(handle_record->file_descriptor, buf, len);
    if(bytes_read < 0)  {
        return -1;
    }

    handle_record->position += (packfile_offset_t)bytes_read;
    return (int)bytes_read;
}

static int pak_rawread(int virtual_file_descriptor, unsigned char *destination, int requested_length, int blocking) {
    packfile_offset_t absolute_position;
    packfile_offset_t end_position;
    packfile_offset_t virtual_file_end;
    int total_read = 0;
    s_packfile_handle *handle_record;

    if(requested_length <= 0)  {
        return 0;
    }

    handle_record = packfile_handle_get_type(&packfile_handle_table, virtual_file_descriptor, PACKFILE_HANDLE_CACHED);
    if(!handle_record || handle_record->position > handle_record->size)  {
        return 0;
    }

    if(!packfile_cached_data_range_is_valid(handle_record->data_start, handle_record->size))  {
        return 0;
    }

    absolute_position = handle_record->data_start + handle_record->position;
    virtual_file_end = handle_record->data_start + handle_record->size;
    if(absolute_position >= virtual_file_end)  {
        return 0;
    }
    if((packfile_size_t)requested_length > virtual_file_end - absolute_position)  {
        requested_length = (int)(virtual_file_end - absolute_position);
    }
    end_position = absolute_position + (packfile_offset_t)requested_length;

    update_filecache_vfd(virtual_file_descriptor);

    while(absolute_position < end_position)  {
        int read_result;
        int pak_block;
        int start_this_block;
        int size_this_block;

        if(!packfile_offset_to_int(absolute_position / CACHEBLOCKSIZE, &pak_block))  {
            break;
        }

        start_this_block = (int)(absolute_position % CACHEBLOCKSIZE);
        size_this_block = CACHEBLOCKSIZE - start_this_block;
        if((packfile_offset_t)size_this_block > (end_position - absolute_position))  {
            size_this_block = (int)(end_position - absolute_position);
        }

        read_result = filecache_readpakblock(destination, pak_block, start_this_block, size_this_block, blocking);
        if(read_result >= 0)  {
            total_read += read_result;
            handle_record->position += read_result;
            update_filecache_vfd(virtual_file_descriptor);
        }
        if(read_result < size_this_block)  {
            break;
        }

        destination += size_this_block;
        absolute_position += size_this_block;
    }
    return total_read;
}

int readpackfileblocking(int fd, void *buf, int len, int blocking) {
    int bytes_read;
    s_packfile_handle *handle_record;

    if(!pak_cache_enabled)  {
        return readPackfile(fd, buf, len);
    }
    if(len < 0)  {
        return -1;
    }
    if(len == 0)  {
        return 0;
    }

    handle_record = packfile_handle_get_type(&packfile_handle_table, fd, PACKFILE_HANDLE_CACHED);
    if(!handle_record)  {
        return -1;
    }
    if(handle_record->position > handle_record->size)  {
        handle_record->position = handle_record->size;
    }
    if((packfile_size_t)len > (handle_record->size - handle_record->position))  {
        len = (int)(handle_record->size - handle_record->position);
    }
    if(len < 1)  {
        return 0;
    }

    update_filecache_vfd(fd);
    bytes_read = pak_rawread(fd, buf, len, blocking);
    if(bytes_read < 0)  {
        bytes_read = 0;
    }
    if(handle_record->position > handle_record->size)  {
        handle_record->position = handle_record->size;
    }
    update_filecache_vfd(fd);
    return bytes_read;
}

int readpackfile_noblock(int handle, void *buf, int len) {
    return readpackfileblocking(handle, buf, len, 0);
}

int readPackfileCached(int handle, void *buf, int len) {
    return readpackfileblocking(handle, buf, len, 1);
}


int closepackfile(int handle) {
    s_packfile_handle *handle_record;
#ifdef VERBOSE
    char *pointsto;
#endif

    handle_record = packfile_handle_get(&packfile_handle_table, handle);
    if(!handle_record)  {
        return -1;
    }
#ifdef VERBOSE
    if(handle_record->type == PACKFILE_HANDLE_CACHED)  {
        pointsto = "closePackfileCached";
    }
    else  {
        pointsto = "closePackfile";
    }
    printf("closepackfile called: h: %d, dest: %s\n", handle, pointsto);
#endif
    if(handle_record->type == PACKFILE_HANDLE_DIRECT)  {
        return closePackfile(handle);
    }
    return closePackfileCached(handle);
}

int closePackfile(int handle) {
    s_packfile_handle *handle_record;

    handle_record = packfile_handle_get_type(&packfile_handle_table, handle, PACKFILE_HANDLE_DIRECT);
    if(!handle_record || handle_record->file_descriptor < 0)  {
        return -1;
    }
    close(handle_record->file_descriptor);
    packfile_handle_release(&packfile_handle_table, handle);
    return 0;
}

int closePackfileCached(int handle) {
    if(!packfile_handle_get_type(&packfile_handle_table, handle, PACKFILE_HANDLE_CACHED))  {
        return -1;
    }
    filecache_setvfd(handle, -1, -1, 0);
    packfile_handle_release(&packfile_handle_table, handle);
    return 0;
}


int seekpackfile(int handle, int offset, int whence) {
    packfile_signed_offset_t position;

    position = seekpackfile64(handle, (packfile_signed_offset_t)offset, whence);
    if(position < 0 || position > INT_MAX)  {
        return -1;
    }
    return (int)position;
}

packfile_signed_offset_t seekpackfile64(int handle, packfile_signed_offset_t offset, int whence) {
    s_packfile_handle *handle_record = packfile_handle_get(&packfile_handle_table, handle);

    if(!handle_record)  {
        return -1;
    }
    if(handle_record->type == PACKFILE_HANDLE_DIRECT)  {
        return seekPackfile64(handle, offset, whence);
    }
    return seekPackfileCached64(handle, offset, whence);
}

packfile_signed_offset_t seekPackfile64(int handle, packfile_signed_offset_t offset, int whence) {
    packfile_signed_offset_t desired_offset;
    packfile_signed_offset_t seek_distance;
    s_packfile_handle *handle_record;

    handle_record = packfile_handle_get_type(&packfile_handle_table, handle, PACKFILE_HANDLE_DIRECT);
    if(!handle_record || handle_record->file_descriptor < 0)  {
        return -1;
    }

    switch(whence)  {
    case SEEK_SET:
        desired_offset = offset;
        break;
    case SEEK_CUR:
        if(offset > 0 && handle_record->position > (packfile_offset_t)INT64_MAX - (packfile_offset_t)offset)  {
            return -1;
        }
        desired_offset = (packfile_signed_offset_t)handle_record->position + offset;
        break;
    case SEEK_END:
        if(offset > 0 && handle_record->size > (packfile_size_t)INT64_MAX - (packfile_size_t)offset)  {
            return -1;
        }
        desired_offset = (packfile_signed_offset_t)handle_record->size + offset;
        break;
    default:
        return -1;
    }

    if(desired_offset < 0)  {
        desired_offset = 0;
    }
    if((packfile_size_t)desired_offset > handle_record->size)  {
        desired_offset = (packfile_signed_offset_t)handle_record->size;
    }

    seek_distance = desired_offset - (packfile_signed_offset_t)handle_record->position;
    if(packfile_seek_fd(handle_record->file_descriptor, seek_distance, SEEK_CUR) < 0)  {
        return -1;
    }
    handle_record->position = (packfile_offset_t)desired_offset;
    return desired_offset;
}

int seekPackfile(int handle, int offset, int whence) {
    packfile_signed_offset_t position;

    position = seekPackfile64(handle, (packfile_signed_offset_t)offset, whence);
    if(position < 0 || position > INT_MAX)  {
        return -1;
    }
    return (int)position;
}

packfile_signed_offset_t seekPackfileCached64(int handle, packfile_signed_offset_t offset, int whence) {
    packfile_signed_offset_t desired_offset;
    s_packfile_handle *handle_record;

    if(!pak_cache_enabled)  {
        return seekPackfile64(handle, offset, whence);
    }
    handle_record = packfile_handle_get_type(&packfile_handle_table, handle, PACKFILE_HANDLE_CACHED);
    if(!handle_record)  {
        return -1;
    }

    switch(whence)  {
    case SEEK_SET:
        desired_offset = offset;
        break;
    case SEEK_CUR:
        desired_offset = (packfile_signed_offset_t)handle_record->position + offset;
        break;
    case SEEK_END:
        desired_offset = (packfile_signed_offset_t)handle_record->size + offset;
        break;
    default:
        return -1;
    }

    if(desired_offset < 0)  {
        desired_offset = 0;
    }
    if((packfile_size_t)desired_offset > handle_record->size)  {
        desired_offset = (packfile_signed_offset_t)handle_record->size;
    }

    handle_record->position = (packfile_offset_t)desired_offset;
    update_filecache_vfd(handle);
    return desired_offset;
}

int seekPackfileCached(int handle, int offset, int whence) {
    packfile_signed_offset_t position;

    position = seekPackfileCached64(handle, (packfile_signed_offset_t)offset, whence);
    if(position < 0 || position > INT_MAX)  {
        return -1;
    }
    return (int)position;
}


void pak_term() {
    if(!pak_initialized)  {
        packfile_release_all_handles();
        return;
    }
    packfile_release_all_handles();
    freefilenamecache();
    if(pak_cdheader != NULL)  {
        free(pak_cdheader);
        pak_cdheader = NULL;
        pak_header = NULL;
    }
    if(pak_cache_enabled)  {
        filecache_term();
    }
    if(pakfd >= 0)  {
        close(pakfd);
    }
    pakfd = -1;
    paksize = 0;
    pak_headerstart = 0;
    pak_headersize = 0;
    pak_footer_size = PAK32_FOOTER_SIZE;
    pak_entry_header_size = PAK32_TABLE_ENTRY_HEADER_SIZE;
    pak_cache_enabled = 1;
    pak_initialized = 0;
}


int pak_init() {
    int file_permission = 666;
    packfile_format format;
    size_t header_bytes_to_cache;
    int pak_sector_count;

    if(pak_initialized)  {
        printf("pak_init already initialized!");
        return 0;
    }

#if WIN || LINUX
    if(isRawData())  {
        pak_initialized = 1;
        packfile_mode(0);
        return 0;
    }
#endif

    pak_cache_enabled = 1;
    packfile_mode(1);

    pakfd = open(packfile, O_RDONLY | O_BINARY, file_permission);
    if(pakfd < 0)  {
        printf("error opening %s (%d) - could not get a valid device descriptor.\n%s\n", packfile, pakfd, strerror(errno));
        return 0;
    }

    if(!packfile_read_format_fd(pakfd, &format))  {
        close(pakfd);
        pakfd = -1;
        return -1;
    }

    /*
    * filecache.c still accepts int-sized pack byte/sector math. To avoid
    * overflow there, use the direct 64-bit reader for any valid pack above
    * the cache limit. Oversized PAK32 packs have already failed format
    * detection before this point.
    */
    if(format.file_size > PACK_CACHE_MAXIMUM_SIZE)  {
        printf("Info: %s exceeds cached pack reader limits; using direct 64-bit reader.\n", packfile);
        close(pakfd);
        pakfd = -1;
        pak_cache_enabled = 0;
        pak_initialized = 1;
        packfile_mode(0);
        return 0;
    }

    pak_footer_size = format.footer_size;
    pak_entry_header_size = format.entry_header_size;
    paksize = format.file_size;
    pak_headerstart = format.table_offset;
    pak_headersize = (size_t)(format.table_end_offset - format.table_offset);

    header_bytes_to_cache = pak_headersize + 1;
    pak_cdheader = malloc(header_bytes_to_cache);
    if(!pak_cdheader)  {
        printf("pak header malloc failed\n");
        close(pakfd);
        pakfd = -1;
        return 0;
    }

    if(packfile_seek_fd_unsigned(pakfd, pak_headerstart, SEEK_SET) < 0 || !packfile_read_exact_fd(pakfd, pak_cdheader, pak_headersize))  {
        printf("unable to read pak header\n");
        free(pak_cdheader);
        pak_cdheader = NULL;
        close(pakfd);
        pakfd = -1;
        return 0;
    }
    pak_header = pak_cdheader;
    pak_header[pak_headersize] = 0;

    pak_sector_count = (int)((paksize + 0x7FF) / 0x800);
    filecache_init(pakfd, pak_sector_count, CACHEBLOCKSIZE, CACHEBLOCKS);
    pak_initialized = 1;
    return (CACHEBLOCKSIZE * CACHEBLOCKS + 64);
}


int packfileeof(int handle) {
    s_packfile_handle *handle_record = packfile_handle_get(&packfile_handle_table, handle);

    if(!handle_record)  {
        return -1;
    }
    return (handle_record->position >= handle_record->size);
}


int packfile_supported(const char *filename) {
    if(stricmp(filename, "menu.pak") != 0)  {
        if(stristr(filename, ".pak"))  {
            return 1;
        }
    }
    return 0;
}


static void packfile_copy_music_track_title(const char *input_path, char output_title[MAX_FILENAME_LEN]) {
    int input_index;
    int last_separator = 0;
    int output_index = 0;

    for(input_index = 0; input_index < (int)strlen(input_path); input_index++)  {
        if((input_path[input_index] == '/') || (input_path[input_index] == '\\'))  {
            last_separator = input_index;
        }
    }
    for(input_index = 0; input_index < (int)strlen(input_path) && output_index < MAX_FILENAME_LEN - 1; input_index++)  {
        if(input_index > last_separator)  {
            output_title[output_index] = input_path[input_index];
            output_index++;
        }
    }
    output_title[output_index] = '\0';
}

/*
* Caskey, Damon V.
* 2026-08-02
*
* Identify WAV and Ogg audio files available to
* the music browser.
*/
static int packfile_entry_is_music_track(const char *entry_name) {
    const char *extension = strrchr(entry_name, '.');

    if(!extension)  {
        return 0;
    }

    return !stricmp(extension, ".wav") ||
           !stricmp(extension, ".ogg") ||
           !stricmp(extension, ".oga");
}

static int packfile_count_music_tracks(int pack_descriptor, const packfile_format *format) {
    packfile_entry entry;
    packfile_offset_t table_position = format->table_offset;
    int track_count = 0;

    while(table_position < format->table_end_offset)  {
        if(!packfile_read_entry_fd(pack_descriptor, format, table_position, &entry))  {
            return -1;
        }

        if(packfile_entry_is_music_track(entry.name) &&
           packfile_asset_size_is_supported(entry.data_size, entry.name))  {
            if(track_count == INT_MAX)  {
                return -1;
            }
            track_count++;
        }

        table_position += entry.record_size;
    }

    return track_count;
}

static int packfile_allocate_music_track_list(fileliststruct *file_list_entry, int track_count) {
    if(track_count <= 0)  {
        file_list_entry->bgmFileName = NULL;
        file_list_entry->bgmTracks = NULL;
        return 1;
    }

    if((size_t)track_count > (SIZE_MAX / sizeof(*file_list_entry->bgmFileName)) ||
       (size_t)track_count > (SIZE_MAX / sizeof(*file_list_entry->bgmTracks)))  {
        return 0;
    }

    file_list_entry->bgmFileName = calloc((size_t)track_count, sizeof(*file_list_entry->bgmFileName));
    file_list_entry->bgmTracks = calloc((size_t)track_count, sizeof(*file_list_entry->bgmTracks));

    if(!file_list_entry->bgmFileName || !file_list_entry->bgmTracks)  {
        free(file_list_entry->bgmFileName);
        free(file_list_entry->bgmTracks);
        file_list_entry->bgmFileName = NULL;
        file_list_entry->bgmTracks = NULL;
        return 0;
    }

    return 1;
}

static void packfile_music_clear_entry(fileliststruct *file_list_entry) {
    free(file_list_entry->bgmFileName);
    free(file_list_entry->bgmTracks);
    file_list_entry->bgmFileName = NULL;
    file_list_entry->bgmTracks = NULL;
    file_list_entry->nTracks = 0;
    file_list_entry->bgmTrack = 0;
}

static int packfile_populate_music_track_list(int pack_descriptor, const packfile_format *format, fileliststruct *file_list_entry) {
    packfile_entry entry;
    packfile_offset_t table_position = format->table_offset;
    int track_index = 0;

    while(table_position < format->table_end_offset)  {
        if(!packfile_read_entry_fd(pack_descriptor, format, table_position, &entry))  {
            return 0;
        }

        if(packfile_entry_is_music_track(entry.name) &&
           packfile_asset_size_is_supported(entry.data_size, entry.name))  {
            if(track_index >= file_list_entry->nTracks)  {
                return 0;
            }
            packfile_copy_music_track_title(entry.name, file_list_entry->bgmFileName[track_index]);
            file_list_entry->bgmTracks[track_index] = table_position;
            track_index++;
        }

        table_position += entry.record_size;
    }

    return track_index == file_list_entry->nTracks;
}

void packfile_music_init(fileliststruct *filelist, int dListTotal) {
    int file_index;

    if(!filelist)  {
        return;
    }

    for(file_index = 0; file_index < dListTotal; file_index++)  {
        filelist[file_index].bgmFileName = NULL;
        filelist[file_index].bgmTracks = NULL;
        filelist[file_index].nTracks = 0;
        filelist[file_index].bgmTrack = 0;
    }
}

void packfile_music_free(fileliststruct *filelist, int dListTotal) {
    int file_index;

    if(!filelist)  {
        return;
    }

    for(file_index = 0; file_index < dListTotal; file_index++)  {
        packfile_music_clear_entry(&filelist[file_index]);
    }
}

void packfile_music_read(fileliststruct *filelist, int dListTotal) {
    packfile_format format;
    int pack_descriptor;
    int file_index;
    int track_count;
    int file_permission = 666;

    for(file_index = 0; file_index < dListTotal; file_index++)  {
        packfile_music_clear_entry(&filelist[file_index]);

        getBasePath(packfile, filelist[file_index].filename, 1);
        if(!stristr(packfile, ".pak"))  {
            continue;
        }

        pack_descriptor = open(packfile, O_RDONLY | O_BINARY, file_permission);
        if(pack_descriptor < 0)  {
            continue;
        }
        if(!packfile_read_format_fd(pack_descriptor, &format))  {
            close(pack_descriptor);
            continue;
        }

        track_count = packfile_count_music_tracks(pack_descriptor, &format);
        if(track_count <= 0)  {
            close(pack_descriptor);
            continue;
        }

        filelist[file_index].nTracks = track_count;
        if(!packfile_allocate_music_track_list(&filelist[file_index], track_count) ||
           !packfile_populate_music_track_list(pack_descriptor, &format, &filelist[file_index]))  {
            packfile_music_clear_entry(&filelist[file_index]);
        }

        close(pack_descriptor);
    }
}


int packfile_music_play(struct fileliststruct *filelist, FILE *bgmFile, int bgmLoop, int curPos, int scrPos) {
    packfile_format format;
    packfile_entry entry;
    fileliststruct *selected_file;
    int pack_descriptor;
    int file_permission = 666;
    packfile_offset_t table_position;

    selected_file = &filelist[curPos + scrPos];
    if(!selected_file->bgmTracks || selected_file->bgmTrack < 0 || selected_file->bgmTrack >= selected_file->nTracks)  {
        return 0;
    }

    getBasePath(packfile, selected_file->filename, 1);
    if(bgmFile)  {
        fclose(bgmFile);
        bgmFile = NULL;
    }

    if(!stristr(packfile, ".pak"))  {
        return 0;
    }

    pack_descriptor = open(packfile, O_RDONLY | O_BINARY, file_permission);
    if(pack_descriptor < 0)  {
        return 0;
    }

    if(!packfile_read_format_fd(pack_descriptor, &format))  {
        close(pack_descriptor);
        return 0;
    }

    table_position = selected_file->bgmTracks[selected_file->bgmTrack];
    if(table_position < format.table_offset || table_position >= format.table_end_offset)  {
        close(pack_descriptor);
        return 0;
    }

    if(!packfile_read_entry_fd(pack_descriptor, &format, table_position, &entry) ||
       !packfile_asset_size_is_supported(entry.data_size, entry.name))  {
        close(pack_descriptor);
        return 0;
    }

    close(pack_descriptor);
    sound_open_music(entry.name, packfile, savedata.musicvol, bgmLoop, 0);
    return 1;
}

#endif
