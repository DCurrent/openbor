/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <locale.h>
#include <math.h>

#include "stringptr.h"
#include "utils.h"
#include "stristr.h"
#include "openbor.h"
#include "packfile.h"

#include <dirent.h>

#ifdef SDL
#include <unistd.h>
#include "sdlport.h"
#include "savepng.h"
#endif

#if _POSIX_VERSION > 0
#include <sys/stat.h>
#endif

#if WII
#include "wiiport.h"
#include "savepng.h"
#endif

#if ANDROID
#include "sdlport.h"
#include "savepng.h"
#endif

#ifdef WIN
#define MKDIR(x) mkdir(x)
#else
#define MKDIR(x) mkdir(x, 0777)
#endif

#ifdef WII
#define CHECK_LOGFILE(type)  type ? fileExists(getFullPath("Logs/OpenBorLog.txt")) : fileExists(getFullPath("Logs/ScriptLog.txt"))
#define OPEN_LOGFILE(type)   type ? fopen(getFullPath("Logs/OpenBorLog.txt"), "wt") : fopen(getFullPath("Logs/ScriptLog.txt"), "wt")
#define APPEND_LOGFILE(type) type ? fopen(getFullPath("Logs/OpenBorLog.txt"), "at") : fopen(getFullPath("Logs/ScriptLog.txt"), "at")
#define READ_LOGFILE(type)   type ? fopen(getFullPath("Logs/OpenBorLog.txt"), "rt") : fopen(getFullPath("Logs/ScriptLog.txt"), "rt")
#define COPY_ROOT_PATH(buf, name) strcpy(buf, rootDir); strcat(buf, name); strcat(buf, "/");
#define COPY_PAKS_PATH(buf, name) strcpy(buf, paksDir); strcat(buf, "/"); strcat(buf, name);
#elif ANDROID
//msmalik681 now using AndroidRoot fuction from sdlport.c to update all android paths.
#define Alog AndroidRoot("Logs/OpenBorLog.txt")
#define Aslog AndroidRoot("Logs/ScriptLog.txt")
#define CHECK_LOGFILE(type)  type ? fileExists(Alog) : fileExists(Aslog)
#define OPEN_LOGFILE(type)   type ? fopen(Alog, "wt") : fopen(Aslog, "wt")
#define APPEND_LOGFILE(type) type ? fopen(Alog, "at") : fopen(Aslog, "at")
#define READ_LOGFILE(type)   type ? fopen(Alog, "rt") : fopen(Aslog, "rt")
#define COPY_ROOT_PATH(buf, name) strncpy(buf, rootDir, strlen(rootDir)); strncat(buf, name, strlen(name)); strncat(buf, "/", 1);
#define COPY_PAKS_PATH(buf, name) strncpy(buf, paksDir, strlen(paksDir)); strncat(buf, "/", 1); strncat(buf, name, strlen(name));
#else
#define CHECK_LOGFILE(type)  type ? fileExists("./Logs/OpenBorLog.txt") : fileExists("./Logs/ScriptLog.txt")
#define OPEN_LOGFILE(type)   type ? fopen("./Logs/OpenBorLog.txt", "wt") : fopen("./Logs/ScriptLog.txt", "wt")
#define APPEND_LOGFILE(type) type ? fopen("./Logs/OpenBorLog.txt", "at") : fopen("./Logs/ScriptLog.txt", "at")
#define READ_LOGFILE(type)   type ? fopen("./Logs/OpenBorLog.txt", "rt") : fopen("./Logs/ScriptLog.txt", "rt")
#define COPY_ROOT_PATH(buf, name) strcpy(buf, "./"); strcat(buf, name); strcat(buf, "/");
#define COPY_PAKS_PATH(buf, name) strcpy(buf, "./Paks/"); strcat(buf, name);
#endif

void debugBuf(unsigned char *buf, size_t size, int columns)
{
    size_t pos = 0;
    int i;
    while(pos < size)
    {
        for(i = 0; i < columns; i++)
        {
            if(pos >= size)
            {
                break;
            }
            printf("%02x", buf[pos]);
            pos++;
        }
        printf("\n");
    }
}

//lowercases a buffer inplace
void lc(char *buf, size_t size)
{
    ptrdiff_t i;
    for(i = 0; i < size; i++)
    {
        buf[i] = tolower((int)buf[i]);
    }
}

// returns position after next newline in buf
size_t getNewLineStart(char *buf)
{
    size_t res = 0;
    while(buf[res] && buf[res] != '\n' && buf[res] != '\r')
    {
        ++res;
    }
    while(buf[res] && (buf[res] == '\n' || buf[res] == '\r'))
    {
        ++res;
    }
    return res;
}

FILE *openborLog = NULL;
FILE *scriptLog = NULL;
char debug_msg[2048];
u32 debug_time = 0;

void getBasePath(char *newName, char *name, int type)
{
    char buf[MAX_BUFFER_LEN] = {""};
    switch(type)
    {
    case 0:
        COPY_ROOT_PATH(buf, name);
        break;
    case 1:
        COPY_PAKS_PATH(buf, name);
        break;
    }
    strcpy(newName, buf);
}


int dirExists(char *dname, int create)
{
    char realName[MAX_LABEL_LEN] = {""};
    DIR	*fd1 = NULL;
    int  fd2 = -1;
    strncpy(realName, dname, MAX_LABEL_LEN - 1);
    fd1 = opendir(realName);
    if(fd1 != NULL)
    {
        closedir(fd1);
        return 1;
    }
    if(create)
    {
        fd2 = MKDIR(realName);
        if(fd2 < 0)
        {
            return 0;
        }
#ifdef DARWIN
        chmod(realName, 0777);
#endif
        return 1;
    }
    return 0;
}

int fileExists(char *fnam)
{
    FILE *handle = NULL;
    if((handle = fopen(fnam, "rb")) == NULL)
    {
        return 0;
    }
    fclose(handle);
    return 1;
}

stringptr *readFromLogFile(int which)
{
    long  size;
    FILE *handle = NULL;
    stringptr *buffer = NULL;
    handle = READ_LOGFILE((which ? OPENBOR_LOG : SCRIPT_LOG));
    if(handle == NULL)
    {
        return NULL;
    }
    fseek(handle, 0, SEEK_END);
    size = ftell(handle);
    rewind(handle);
    if (size == 0)
    {
        goto CLOSE_AND_QUIT;
    }
    // allocate memory to contain the whole file:
    //buffer = (char*)malloc(sizeof(char)*(size+1)); // alloc one additional byte for the
    buffer = new_string(size);
    if(buffer == NULL)
    {
        goto CLOSE_AND_QUIT;
    }
    fread(buffer->ptr, 1, size, handle);
CLOSE_AND_QUIT:
    fclose(handle);
    return buffer;
}


void writeToLogFile(const char *msg, ...)
{
    va_list arglist;
    if(openborLog == NULL)
    {
        openborLog = OPEN_LOGFILE(OPENBOR_LOG);
        if(openborLog == NULL)
        {
            return;
        }
    }
    va_start(arglist, msg);
    vfprintf(openborLog, msg, arglist);
    va_end(arglist);
    fflush(openborLog);
}

void writeToScriptLog(const char *msg)
{
    if(scriptLog == NULL)
    {
        scriptLog = OPEN_LOGFILE(SCRIPT_LOG);
        if(scriptLog == NULL)
        {
            return;
        }
    }
    fwrite(msg, 1, strlen(msg), scriptLog);
    fflush(scriptLog);
}

void debug_printf(char *format, ...)
{
    va_list arglist;

    va_start(arglist, format);
    vsprintf(debug_msg, format, arglist);
    va_end(arglist);

    debug_time = 0xFFFFFFFF;
}

void *checkAlloc(void *ptr, size_t size, const char *func, const char *file, int line)
{
    if (size > 0 && ptr == NULL)
    {
        writeToLogFile("\n\n********** An Error Occurred **********"
                       "\n*            Shutting Down            *\n\n");
        writeToLogFile("Out of memory!\n");
        writeToLogFile("Allocation of size %i failed in function '%s' at %s:%i.\n", size, func, file, line);
#if LINUX && !DARWIN
        writeToLogFile("Memory usage at exit: %u\n", mallinfo2().arena);
#else
        writeToLogFile("Memory usage at exit: %u\n", getUsedRam(BYTES));
#endif
        borExit(2);
    }
    return ptr;
}

// replacement for assert that writes the error to the log file
void exitIfFalse(int value, const char *assertion, const char *func, const char *file, int line)
{
    if(!value)
    {
        writeToLogFile("\n\n********** An Error Occurred **********"
                       "\n*            Shutting Down            *\n\n");
        writeToLogFile("Assertion `%s' failed in function '%s' at %s:%i.\n", assertion, func, file, line);
        writeToLogFile("This is an OpenBOR bug.  Please report this at www.chronocrash.com.\n\n");
        borExit(1);
    }
}

// gives the same behavior as the assert macro defined by C, which we can't use directly since
// we redefine the assert macro to exitIfFalse
void abortIfFalse(int value, const char *assertion, const char *func, const char *file, int line)
{
    if(!value)
    {
        fprintf(stderr, "%s:%i: %s: Assertion `%s' failed.\n", file, line, func, assertion);
        fflush(stderr);
        abort();
    }
}

void getPakName(char *name, int type)
{

    int i, x, y;
    char mod[256] = {""};

    memcpy(mod, packfile, strlen(packfile) - 4);

    switch(type)
    {
    case 0:
        strcat(mod, ".sav");
        break;
    case 1:
        strcat(mod, ".hi");
        break;
    case 2:
        strcat(mod, ".scr");
        break;
    case 3:
        strcat(mod, ".inp");
        break;
    case 4:
        strcat(mod, ".cfg");
        break;
    default:
        // Loose extension!
        break;
    }

    x = 0;
    for(i = 0; i < (int)strlen(mod); i++)
    {
        if((mod[i] == '/') || (mod[i] == '\\'))
        {
            x = i;
        }
    }
    y = 0;
    for(i = 0; i < (int)strlen(mod); i++)
    {
        // For packfiles without '/'
        if(x == 0)
        {
            name[y] = mod[i];
            y++;
        }
        // For packfiles with '/'
        if(x != 0 && i > x)
        {
            name[y] = mod[i];
            y++;
        }
    }
    name[y] = 0;
}

void screenshot(s_screen *vscreen, unsigned char *pal, int ingame)
{
    int	 shotnum = 0;
    char shotname[1024] = {""};
    char modname[MAX_FILENAME_LEN]  = {""};

    getPakName(modname, 99);
    do
    {
#if SDL || WII
        sprintf(shotname, "%s/%s - %04u.png", screenShotsDir, modname, shotnum);
#else
        sprintf(shotname, "./ScreenShots/%s - %04u.png", modname, shotnum);
#endif
        ++shotnum;
    }
    while(fileExists(shotname) && shotnum < 10000);

    if(shotnum < 10000)
    {
        savepng(shotname, vscreen, pal);
    }
    if(ingame)
    {
        debug_printf("Saved %s", shotname);
    }
}

unsigned readlsb32(const unsigned char *src)
{
    return
        ((((unsigned)(src[0])) & 0xFF) <<  0) |
        ((((unsigned)(src[1])) & 0xFF) <<  8) |
        ((((unsigned)(src[2])) & 0xFF) << 16) |
        ((((unsigned)(src[3])) & 0xFF) << 24);
}

// Binary Search: by Whyte Dragon (old one has a bad computational complexity + bugged)
int searchList(const char *list[], const char *value, int length)
{
    int low = 0, high = length-1, mid = 0;

    //printf("\n\n searchList(%p, %s, %d)", &list, value, length);

    while(low <= high)
    {
        mid = (low + high)/2;
        if( stricmp(list[mid], value) < 0 )
        {
            low = mid + 1;
        }
        else if( stricmp(list[mid], value) == 0 )
        {
            return mid;
        }
        else if( stricmp(list[mid], value) > 0 )
        {
            high = mid-1;
        }

    }

    return -1;
}

char *commaprint(u64 n)
{
    static int comma = '\0';
    static char retbuf[32];
    char *p = &retbuf[sizeof(retbuf) - 1];
    int i = 0;

    if(comma == '\0')
    {
        #ifndef ANDROID
        struct lconv *lcp = localeconv();
        if(lcp != NULL)
        {
            if(lcp->thousands_sep != NULL &&
                    *lcp->thousands_sep != '\0')
            {
                comma = *lcp->thousands_sep;
            }
            else
            {
                comma = ',';
            }
        }
        #else
        comma = ',';
        #endif
    }

    *p = '\0';

    do
    {
        if(i % 3 == 0 && i != 0)
        {
            *--p = comma;
        }
        *--p = '0' + n % 10;
        n /= 10;
        i++;
    }
    while(n != 0);

    return p;
}

char* multistrcatsp(char* buf, ...)
{
    va_list vl;
    int c = 0;
    char* str;

    va_start(vl,buf);

    while( (str = va_arg(vl,char*)) != NULL )
    {
        if(c == 0) strcpy(buf,str);
        else
        {
            strcat(buf," ");
            strcat(buf,str);
        }
        ++c;
    }

    va_end(vl);

    return buf;
}

char* safe_strncpy(char* dst, const char* src, size_t size)
{
	if (size > 0) {
		register char *d = dst;
		register const char *s = src;

		do {
			if ((*d++ = *s++) == 0) {
				/* NUL pad the remaining n-1 bytes */
				while (--size > 0) *d++ = 0;
				break;
			}
		} while (--size > 0);
		*d = 0;
	}
	return (dst);
}

void get_time_string(char buffer[], unsigned buffer_size, time_t timestamp, char* pattern)
{
    struct tm* tm_info;
    tm_info = localtime(&timestamp);
    strftime(buffer, buffer_size, pattern, tm_info);
    return;
}

void get_now_string(char buffer[], unsigned buffer_size, char* pattern)
{
    time_t rawtime;
    time(&rawtime);
    get_time_string(buffer, buffer_size, rawtime, pattern);
    return;
}

int safe_stricmp(const char *s1, const char *s2)
{
    for (;;) {
        if (*s1 != *s2) {
            int c1 = toupper((unsigned char)*s1);
            int c2 = toupper((unsigned char)*s2);

            if (c2 != c1) {
                return c2 > c1 ? -1 : 1;
            }
        } else {
            if (*s1 == '\0') {
                return 0;
            }
        }
        ++s1;
        ++s2;
    }
}

int safe_strnicmp(const char *s1, const char *s2, size_t n)
{
    for (;;) {
        if (n-- == 0) {
            return 0;
        }
        if (*s1 != *s2) {
            int c1 = toupper((unsigned char)*s1);
            int c2 = toupper((unsigned char)*s2);

            if (c2 != c1) {
                return c2 > c1 ? -1 : 1;
            }
        } else {
            if (*s1 == '\0') {
                return 0;
            }
        }
        ++s1;
        ++s2;
    }
}

//! Increase or Decrease an array à la \e vector
/**
	\param f_caller : name of the calling function for logging purpose
	\param array : the array to consider
	\param new_size : new size needed for the array (in BYTE) :
		-# if new_size <= 0 : Deallocation of the array
		-# new_size < \a curr_size_allocated - \a grow_step => Decrease of the array
		-# new_size >= \a curr_size_allocated => Increase of the array
	\param curr_size_allocated : current allocated size to the array (in BYTE)
	\param grow_step : bloc size of expansion of the array (in BYTE)
*/
void Array_Check_Size( const char *f_caller, char **array, int new_size, int *curr_size_allocated, int grow_step )
{
    // Deallocation
    if( new_size <= 0 )
    {
        if( *array != NULL )
        {
            free(*array);
            *array = NULL;
        }
        *curr_size_allocated = 0;
    }

    // First allocation
    else if( *array == NULL )
    {
        *curr_size_allocated = grow_step;
        *array = malloc(*curr_size_allocated );
        if( *array == NULL)
        {
            borShutdown(1, "Out Of Memory!  Failed in %s\n", f_caller);
        }
        memset( *array, 0, *curr_size_allocated );
        return;
    }

    // No need to decrease or increase the array
    else if( new_size > (*curr_size_allocated - grow_step ) && new_size <= *curr_size_allocated )
    {
        return;
    }

    //-------------------------------------------
    // Must increase or decrease the array size

    int old_size = *curr_size_allocated;

    // Recompute needed size
    *curr_size_allocated = ((int)ceil((float)new_size / (float)grow_step)) * grow_step;

    // Alloc a new array
    void *copy = malloc(*curr_size_allocated );
    if(copy == NULL)
    {
        borShutdown(1, "Out Of Memory!  Failed in %s\n", f_caller);
    }

    // Copy the previous content of the array
    memcpy(copy, *array, ( (old_size < new_size) ? old_size : new_size) );

    // Init the new allocations
    if( old_size < *curr_size_allocated )
    {
        memset( copy + old_size, 0, *curr_size_allocated - old_size );
    }

    // Free previous array memory
    free(*array);

    // ReAssign the new allocated array
    *array = copy;
}

