/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2013 OpenBOR Team
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

#ifndef DC
#include <dirent.h>
#endif

#ifdef _POSIX_SOURCE
#include <sys/stat.h>
#endif

#ifdef SDL
#include <unistd.h>
#include "sdlport.h"
#include "savepng.h"
#endif

#ifdef DC
#include "dcport.h"
#endif

#ifdef PSP
#include "image.h"
#endif

#if WII
#include "wiiport.h"
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
#define COPY_ROOT_PATH(buf, name) strcpy(buf, rootDir); strncat(buf, name, strlen(name)); strncat(buf, "/", 1);
#define COPY_PAKS_PATH(buf, name) strncpy(buf, paksDir, strlen(paksDir)); strncat(buf, "/", 1); strncat(buf, name, strlen(name));
#elif ANDROID
#define CHECK_LOGFILE(type)  type ? fileExists("/mnt/sdcard/OpenBOR/Logs/OpenBorLog.txt") : fileExists("/mnt/sdcard/OpenBOR/Logs/ScriptLog.txt")
#define OPEN_LOGFILE(type)   type ? fopen("/mnt/sdcard/OpenBOR/Logs/OpenBorLog.txt", "wt") : fopen("/mnt/sdcard/OpenBOR/Logs/ScriptLog.txt", "wt")
#define APPEND_LOGFILE(type) type ? fopen("/mnt/sdcard/OpenBOR/Logs/OpenBorLog.txt", "at") : fopen("/mnt/sdcard/OpenBOR/Logs/ScriptLog.txt", "at")
#define READ_LOGFILE(type)   type ? fopen("/mnt/sdcard/OpenBOR/Logs/OpenBorLog.txt", "rt") : fopen("/mnt/sdcard/OpenBOR/Logs/ScriptLog.txt", "rt")
#define COPY_ROOT_PATH(buf, name) strncpy(buf, "/mnt/sdcard/OpenBOR/", 20); strncat(buf, name, strlen(name)); strncat(buf, "/", 1);
#define COPY_PAKS_PATH(buf, name) strncpy(buf, "/mnt/sdcard/OpenBOR/Paks/", 25); strncat(buf, name, strlen(name));
#else
#define CHECK_LOGFILE(type)  type ? fileExists("./Logs/OpenBorLog.txt") : fileExists("./Logs/ScriptLog.txt")
#define OPEN_LOGFILE(type)   type ? fopen("./Logs/OpenBorLog.txt", "wt") : fopen("./Logs/ScriptLog.txt", "wt")
#define APPEND_LOGFILE(type) type ? fopen("./Logs/OpenBorLog.txt", "at") : fopen("./Logs/ScriptLog.txt", "at")
#define READ_LOGFILE(type)   type ? fopen("./Logs/OpenBorLog.txt", "rt") : fopen("./Logs/ScriptLog.txt", "rt")
#define COPY_ROOT_PATH(buf, name) strncpy(buf, "./", 2); strncat(buf, name, strlen(name)); strncat(buf, "/", 1);
#define COPY_PAKS_PATH(buf, name) strncpy(buf, "./Paks/", 7); strncat(buf, name, strlen(name));
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
#ifndef DC
    char buf[128] = {""};
    switch(type)
    {
    case 0:
        COPY_ROOT_PATH(buf, name);
        break;
    case 1:
        COPY_PAKS_PATH(buf, name);
        break;
    }
    memcpy(newName, buf, sizeof(buf));
#else
    memcpy(newName, name, 128);
#endif
}



#ifndef DC
int dirExists(char *dname, int create)
{
    char realName[128] = {""};
    DIR	*fd1 = NULL;
    int  fd2 = -1;
    strncpy(realName, dname, 128);
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
#endif

void writeToLogFile(const char *msg, ...)
{
    va_list arglist;

#ifdef DC
    va_start(arglist, msg);
    vfprintf(stdout, msg, arglist);
    va_end(arglist);
    fflush(stdout);
#else
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
#endif
}

void writeToScriptLog(const char *msg)
{
#ifndef DC
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
#endif
}

void debug_printf(char *format, ...)
{
    va_list arglist;

    va_start(arglist, format);
    vsprintf(debug_msg, format, arglist);
    va_end(arglist);

    debug_time = 0xFFFFFFFF;
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
        exit(1);
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

    strncpy(mod, packfile, strlen(packfile) - 4);

    switch(type)
    {
    case 0:
        strncat(mod, ".sav", 4);
        break;
    case 1:
        strncat(mod, ".hi", 3);
        break;
    case 2:
        strncat(mod, ".scr", 4);
        break;
    case 3:
        strncat(mod, ".inp", 4);
        break;
    case 4:
        strncat(mod, ".cfg", 4);
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
#ifndef DC
    int	 shotnum = 0;
    char shotname[128] = {""};
    char modname[128]  = {""};

    getPakName(modname, 99);
#ifdef PSP
    if(dirExists("ms0:/PICTURE/", 1) && dirExists("ms0:/PICTURE/Beats Of Rage/", 1))
    {
#endif
        do
        {
#if PSP
            sprintf(shotname, "ms0:/PICTURE/Beats Of Rage/%s - ScreenShot - %02u.png", modname, shotnum);
#elif SDL || WII
            sprintf(shotname, "%s/%s - %04u.png", screenShotsDir, modname, shotnum);
#else
            sprintf(shotname, "./ScreenShots/%s - %04u.png", modname, shotnum);
#endif
            ++shotnum;
        }
        while(fileExists(shotname) && shotnum < 10000);

#ifdef PSP
        if(shotnum < 10000)
        {
            saveImage(shotname);
        }
#else
        if(shotnum < 10000)
        {
            savepng(shotname, vscreen, pal);
        }
#endif
        if(ingame)
        {
            debug_printf("Saved %s", shotname);
        }
#ifdef PSP
    }
#endif
#endif
}

unsigned readlsb32(const unsigned char *src)
{
    return
        ((((unsigned)(src[0])) & 0xFF) <<  0) |
        ((((unsigned)(src[1])) & 0xFF) <<  8) |
        ((((unsigned)(src[2])) & 0xFF) << 16) |
        ((((unsigned)(src[3])) & 0xFF) << 24);
}

// Optimized search in an arranged string table, return the index
int searchList(const char *list[], const char *value, int length)
{
    int i;
    int a = 0;
    int b = length / 2;
    int c = length - 1;
    int v = value[0];

    // We must convert uppercase values to lowercase,
    // since this is how every command is written in
    // our source.  Refer to an ASCII Chart
    if(v >= 0x41 && v <= 0x5A)
    {
        v += 0x20;
    }

    // Index value equals middle value,
    // Lets search starting from center.
    if(v == list[b][0])
    {
        if(stricmp(list[b], value) == 0)
        {
            return b;
        }

        // Search Down the List.
        if(v == list[b - 1][0])
        {
            for(i = b - 1 ; i >= 0; i--)
            {
                if(stricmp(list[i], value) == 0)
                {
                    return i;
                }
                if(v != list[i - 1][0])
                {
                    break;
                }
            }
        }

        // Search Up the List.
        if(v == list[b + 1][0])
        {
            for(i = b + 1; i < length; i++)
            {
                if(stricmp(list[i], value) == 0)
                {
                    return i;
                }
                if(v != list[i + 1][0])
                {
                    break;
                }
            }
        }

        // No match, return failure.
        goto searchListFailed;
    }

    // Define the starting point.
    if(v >= list[b + 1][0])
    {
        a = b + 1;
    }
    else if(v <= list[b - 1][0])
    {
        c = b - 1;
    }
    else
    {
        goto searchListFailed;
    }

    // Search Up from starting point.
    for(i = a; i <= c; i++)
    {
        if(v == list[i][0])
        {
            if(stricmp(list[i], value) == 0)
            {
                return i;
            }
            if(v != list[i + 1][0])
            {
                break;
            }
        }
    }

searchListFailed:

    // The search failed!
    // On five reasons for failure!
    // 1. Is the list in alphabetical order?
    // 2. Is the first letter lowercase in list?
    // 3. Does the value exist in the list?
    // 4. Is it a typo?
    // 5. Is it a text file error?
    return -1;
}

char *commaprint(u64 n)
{
    static int comma = '\0';
    static char retbuf[30];
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
void
Array_Check_Size( const char *f_caller, char **array, int new_size, int *curr_size_allocated, int grow_step )
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
            shutdown(1, "Out Of Memory!  Failed in %s\n", f_caller);
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
        shutdown(1, "Out Of Memory!  Failed in %s\n", f_caller);
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


// don't ask; go to Wikipedia if you have no fear:
// http://en.wikipedia.org/wiki/Fast_inverse_square_root
float invsqrt(float x)
{
    float xhalf = 0.5f * x;
    union
    {
        float f;
        int i;
    } fi;
    fi.f = x;
    fi.i = 0x5f375a86 - (fi.i >> 1); // gives initial guess y0
    x = fi.f; // convert bits BACK to float
    x = x * (1.5f - xhalf * x * x); // Newton step, repeating increases accuracy
    //x = x*(1.5f-xhalf*x*x);
    //x = x*(1.5f-xhalf*x*x);
    return x;
}

//sin cos tables
//sin cos tables
float sin_table[] = //360
{
    0, 0.01745240643728351, 0.03489949670250097, 0.05233595624294383, 0.0697564737441253, 0.08715574274765816, 0.10452846326765346, 0.12186934340514747, 0.13917310096006544, 0.15643446504023087, 0.17364817766693033, 0.1908089953765448, 0.20791169081775931, 0.224951054343865, 0.24192189559966773, 0.25881904510252074, 0.27563735581699916, 0.29237170472273677, 0.3090169943749474, 0.32556815445715664, 0.3420201433256687, 0.35836794954530027, 0.374606593415912, 0.3907311284892737, 0.40673664307580015, 0.42261826174069944, 0.4383711467890774, 0.45399049973954675, 0.4694715627858908, 0.48480962024633706, 0.49999999999999994, 0.5150380749100542, 0.5299192642332049, 0.5446390350150271, 0.5591929034707469, 0.573576436351046, 0.5877852522924731, 0.6018150231520483, 0.6156614753256582, 0.6293203910498374, 0.6427876096865392, 0.6560590289905072, 0.6691306063588582, 0.6819983600624985, 0.6946583704589972, 0.7071067811865475, 0.7193398003386511, 0.7313537016191705, 0.7431448254773941, 0.754709580222772, 0.766044443118978, 0.7771459614569708, 0.788010753606722, 0.7986355100472928, 0.8090169943749474, 0.8191520442889918, 0.8290375725550417, 0.8386705679454239, 0.848048096156426, 0.8571673007021122, 0.8660254037844386, 0.8746197071393957, 0.8829475928589269, 0.8910065241883678, 0.898794046299167, 0.9063077870366499, 0.9135454576426009, 0.9205048534524403, 0.9271838545667874, 0.9335804264972017, 0.9396926207859083, 0.9455185755993167, 0.9510565162951535, 0.9563047559630354, 0.9612616959383189, 0.9659258262890683, 0.9702957262759965, 0.9743700647852352, 0.9781476007338056, 0.981627183447664, 0.984807753012208, 0.9876883405951378, 0.9902680687415702, 0.992546151641322, 0.9945218953682733, 0.9961946980917455, 0.9975640502598242, 0.9986295347545738, 0.9993908270190958, 0.9998476951563913, 1, 0.9998476951563913, 0.9993908270190958, 0.9986295347545738, 0.9975640502598242, 0.9961946980917455, 0.9945218953682734, 0.9925461516413221, 0.9902680687415704, 0.9876883405951377, 0.984807753012208, 0.981627183447664, 0.9781476007338057, 0.9743700647852352, 0.9702957262759965, 0.9659258262890683, 0.9612616959383189, 0.9563047559630355, 0.9510565162951536, 0.9455185755993168, 0.9396926207859084, 0.9335804264972017, 0.9271838545667874, 0.9205048534524404, 0.913545457642601, 0.90630778703665, 0.8987940462991669, 0.8910065241883679, 0.8829475928589271, 0.8746197071393958, 0.8660254037844387, 0.8571673007021123, 0.8480480961564261, 0.8386705679454239, 0.8290375725550417, 0.819152044288992, 0.8090169943749474, 0.7986355100472927, 0.788010753606722, 0.777145961456971, 0.766044443118978, 0.7547095802227718, 0.7431448254773942, 0.7313537016191706, 0.7193398003386514, 0.7071067811865476, 0.6946583704589971, 0.6819983600624986, 0.6691306063588583, 0.6560590289905073, 0.6427876096865395, 0.6293203910498377, 0.6156614753256584, 0.6018150231520482, 0.5877852522924732, 0.5735764363510464, 0.5591929034707469, 0.544639035015027, 0.5299192642332049, 0.5150380749100544, 0.49999999999999994, 0.48480962024633717, 0.4694715627858911, 0.45399049973954686, 0.4383711467890773, 0.4226182617406995, 0.40673664307580043, 0.39073112848927416, 0.37460659341591223, 0.3583679495453002, 0.3420201433256689, 0.32556815445715703, 0.3090169943749475, 0.29237170472273704, 0.27563735581699966, 0.258819045102521, 0.24192189559966773, 0.22495105434386478, 0.20791169081775931, 0.19080899537654497, 0.17364817766693027, 0.15643446504023098, 0.13917310096006574, 0.12186934340514754, 0.10452846326765373, 0.08715574274765864, 0.06975647374412552, 0.05233595624294381, 0.0348994967025007, 0.01745240643728344, 1.2246063538223772e-16, -0.017452406437283192, -0.0348994967025009, -0.052335956242943564, -0.06975647374412483, -0.08715574274765794, -0.10452846326765305, -0.12186934340514774, -0.13917310096006552, -0.15643446504023073, -0.17364817766693047, -0.19080899537654472, -0.20791169081775906, -0.22495105434386497, -0.2419218955996675, -0.25881904510252035, -0.275637355816999, -0.2923717047227364, -0.30901699437494773, -0.32556815445715675, -0.34202014332566865, -0.35836794954530043, -0.374606593415912, -0.39073112848927355, -0.4067366430757998, -0.4226182617406993, -0.43837114678907707, -0.45399049973954625, -0.46947156278589086, -0.48480962024633694, -0.5000000000000001, -0.5150380749100542, -0.5299192642332048, -0.5446390350150271, -0.5591929034707467, -0.5735764363510458, -0.587785252292473, -0.601815023152048, -0.6156614753256578, -0.6293203910498376, -0.6427876096865392, -0.6560590289905074, -0.6691306063588582, -0.6819983600624984, -0.6946583704589974, -0.7071067811865475, -0.7193398003386509, -0.7313537016191701, -0.743144825477394, -0.7547095802227717, -0.7660444431189779, -0.7771459614569711, -0.7880107536067221, -0.7986355100472928, -0.8090169943749473, -0.8191520442889916, -0.8290375725550414, -0.838670567945424, -0.848048096156426, -0.8571673007021121, -0.8660254037844384, -0.874619707139396, -0.882947592858927, -0.8910065241883678, -0.8987940462991668, -0.9063077870366497, -0.913545457642601, -0.9205048534524403, -0.9271838545667873, -0.9335804264972016, -0.9396926207859082, -0.9455185755993168, -0.9510565162951535, -0.9563047559630353, -0.961261695938319, -0.9659258262890683, -0.9702957262759965, -0.9743700647852351, -0.9781476007338056, -0.9816271834476639, -0.984807753012208, -0.9876883405951377, -0.9902680687415704, -0.9925461516413221, -0.9945218953682734, -0.9961946980917455, -0.9975640502598242, -0.9986295347545738, -0.9993908270190956, -0.9998476951563913, -1, -0.9998476951563913, -0.9993908270190958, -0.9986295347545738, -0.9975640502598243, -0.9961946980917455, -0.9945218953682734, -0.992546151641322, -0.9902680687415704, -0.9876883405951378, -0.9848077530122081, -0.9816271834476641, -0.9781476007338058, -0.9743700647852352, -0.9702957262759966, -0.9659258262890682, -0.9612616959383188, -0.9563047559630354, -0.9510565162951536, -0.945518575599317, -0.9396926207859085, -0.9335804264972021, -0.9271838545667874, -0.9205048534524405, -0.9135454576426008, -0.9063077870366499, -0.898794046299167, -0.8910065241883679, -0.8829475928589271, -0.8746197071393961, -0.8660254037844386, -0.8571673007021123, -0.8480480961564262, -0.8386705679454243, -0.8290375725550421, -0.8191520442889918, -0.8090169943749476, -0.798635510047293, -0.7880107536067218, -0.7771459614569708, -0.7660444431189781, -0.7547095802227722, -0.7431448254773946, -0.731353701619171, -0.7193398003386517, -0.7071067811865477, -0.6946583704589976, -0.6819983600624982, -0.6691306063588581, -0.6560590289905074, -0.6427876096865396, -0.6293203910498378, -0.6156614753256588, -0.6018150231520483, -0.5877852522924734, -0.5735764363510465, -0.5591929034707473, -0.544639035015027, -0.5299192642332058, -0.5150380749100545, -0.5000000000000004, -0.4848096202463369, -0.4694715627858908, -0.45399049973954697, -0.438371146789077, -0.4226182617407, -0.40673664307580015, -0.3907311284892747, -0.37460659341591235, -0.35836794954530077, -0.3420201433256686, -0.32556815445715753, -0.3090169943749476, -0.29237170472273627, -0.2756373558169998, -0.2588190451025207, -0.24192189559966787, -0.22495105434386534, -0.20791169081775987, -0.19080899537654466, -0.17364817766693127, -0.1564344650402311, -0.13917310096006588, -0.12186934340514811, -0.10452846326765341, -0.08715574274765832, -0.06975647374412476, -0.05233595624294437, -0.034899496702500823, -0.01745240643728445
};
float cos_table[] = //360
{
    1, 0.9998476951563913, 0.9993908270190958, 0.9986295347545738, 0.9975640502598242, 0.9961946980917455, 0.9945218953682733, 0.992546151641322, 0.9902680687415704, 0.9876883405951378, 0.984807753012208, 0.981627183447664, 0.9781476007338057, 0.9743700647852352, 0.9702957262759965, 0.9659258262890683, 0.9612616959383189, 0.9563047559630354, 0.9510565162951535, 0.9455185755993168, 0.9396926207859084, 0.9335804264972017, 0.9271838545667874, 0.9205048534524404, 0.9135454576426009, 0.9063077870366499, 0.898794046299167, 0.8910065241883679, 0.882947592858927, 0.8746197071393957, 0.8660254037844387, 0.8571673007021123, 0.848048096156426, 0.838670567945424, 0.8290375725550416, 0.8191520442889918, 0.8090169943749474, 0.7986355100472928, 0.788010753606722, 0.7771459614569709, 0.766044443118978, 0.7547095802227721, 0.7431448254773942, 0.7313537016191706, 0.7193398003386512, 0.7071067811865476, 0.6946583704589974, 0.6819983600624985, 0.6691306063588582, 0.6560590289905073, 0.6427876096865394, 0.6293203910498375, 0.6156614753256583, 0.6018150231520484, 0.5877852522924731, 0.5735764363510462, 0.5591929034707468, 0.5446390350150272, 0.5299192642332049, 0.5150380749100544, 0.5000000000000001, 0.4848096202463371, 0.46947156278589086, 0.4539904997395468, 0.43837114678907746, 0.42261826174069944, 0.4067366430758002, 0.39073112848927394, 0.37460659341591196, 0.3583679495453004, 0.3420201433256688, 0.32556815445715675, 0.30901699437494745, 0.29237170472273677, 0.27563735581699916, 0.25881904510252074, 0.2419218955996679, 0.22495105434386492, 0.20791169081775945, 0.19080899537654491, 0.17364817766693041, 0.15643446504023092, 0.1391731009600657, 0.12186934340514749, 0.10452846326765346, 0.08715574274765814, 0.06975647374412545, 0.052335956242943966, 0.03489949670250108, 0.017452406437283376, 6.123031769111886e-17, -0.017452406437283477, -0.03489949670250073, -0.05233595624294362, -0.06975647374412533, -0.08715574274765823, -0.10452846326765333, -0.12186934340514736, -0.13917310096006535, -0.15643446504023103, -0.1736481776669303, -0.1908089953765448, -0.20791169081775912, -0.2249510543438648, -0.24192189559966778, -0.25881904510252085, -0.27563735581699905, -0.29237170472273666, -0.30901699437494734, -0.3255681544571564, -0.3420201433256687, -0.35836794954530027, -0.37460659341591207, -0.3907311284892736, -0.40673664307580004, -0.42261826174069933, -0.4383711467890775, -0.4539904997395467, -0.46947156278589053, -0.484809620246337, -0.4999999999999998, -0.5150380749100543, -0.5299192642332048, -0.5446390350150271, -0.5591929034707467, -0.5735764363510458, -0.587785252292473, -0.6018150231520484, -0.6156614753256583, -0.6293203910498373, -0.6427876096865394, -0.6560590289905075, -0.6691306063588582, -0.6819983600624984, -0.694658370458997, -0.7071067811865475, -0.7193398003386512, -0.7313537016191705, -0.743144825477394, -0.754709580222772, -0.7660444431189779, -0.7771459614569707, -0.7880107536067219, -0.7986355100472929, -0.8090169943749473, -0.8191520442889916, -0.8290375725550416, -0.8386705679454242, -0.848048096156426, -0.8571673007021122, -0.8660254037844387, -0.8746197071393957, -0.8829475928589268, -0.8910065241883678, -0.898794046299167, -0.9063077870366499, -0.9135454576426008, -0.9205048534524401, -0.9271838545667873, -0.9335804264972017, -0.9396926207859083, -0.9455185755993167, -0.9510565162951535, -0.9563047559630354, -0.9612616959383187, -0.9659258262890682, -0.9702957262759965, -0.9743700647852352, -0.9781476007338057, -0.981627183447664, -0.984807753012208, -0.9876883405951377, -0.9902680687415702, -0.992546151641322, -0.9945218953682733, -0.9961946980917455, -0.9975640502598242, -0.9986295347545738, -0.9993908270190958, -0.9998476951563913, -1, -0.9998476951563913, -0.9993908270190958, -0.9986295347545738, -0.9975640502598243, -0.9961946980917455, -0.9945218953682734, -0.992546151641322, -0.9902680687415702, -0.9876883405951378, -0.984807753012208, -0.981627183447664, -0.9781476007338057, -0.9743700647852352, -0.9702957262759965, -0.9659258262890684, -0.9612616959383189, -0.9563047559630355, -0.9510565162951535, -0.9455185755993167, -0.9396926207859084, -0.9335804264972017, -0.9271838545667874, -0.9205048534524404, -0.9135454576426011, -0.90630778703665, -0.8987940462991671, -0.8910065241883681, -0.8829475928589269, -0.8746197071393958, -0.8660254037844386, -0.8571673007021123, -0.8480480961564261, -0.838670567945424, -0.8290375725550418, -0.819152044288992, -0.8090169943749476, -0.798635510047293, -0.7880107536067222, -0.7771459614569708, -0.766044443118978, -0.7547095802227719, -0.7431448254773942, -0.7313537016191706, -0.7193398003386511, -0.7071067811865477, -0.6946583704589976, -0.6819983600624989, -0.6691306063588585, -0.6560590289905076, -0.6427876096865395, -0.6293203910498372, -0.6156614753256581, -0.6018150231520483, -0.5877852522924732, -0.5735764363510464, -0.5591929034707472, -0.544639035015027, -0.529919264233205, -0.5150380749100545, -0.5000000000000004, -0.48480962024633683, -0.46947156278589075, -0.4539904997395469, -0.43837114678907773, -0.42261826174069994, -0.4067366430758001, -0.3907311284892738, -0.3746065934159123, -0.3583679495453007, -0.3420201433256694, -0.32556815445715664, -0.30901699437494756, -0.2923717047227371, -0.2756373558169989, -0.25881904510252063, -0.24192189559966778, -0.22495105434386525, -0.2079116908177598, -0.19080899537654547, -0.17364817766693033, -0.15643446504023103, -0.13917310096006494, -0.12186934340514717, -0.10452846326765336, -0.08715574274765825, -0.06975647374412558, -0.052335956242944306, -0.03489949670250165, -0.017452406437283498, -1.836909530733566e-16, 0.01745240643728313, 0.03489949670250128, 0.052335956242943946, 0.06975647374412522, 0.08715574274765789, 0.10452846326765298, 0.12186934340514768, 0.13917310096006546, 0.15643446504023067, 0.17364817766692997, 0.19080899537654425, 0.20791169081775856, 0.22495105434386492, 0.24192189559966745, 0.25881904510252113, 0.2756373558169994, 0.2923717047227367, 0.30901699437494723, 0.3255681544571563, 0.34202014332566816, 0.35836794954529954, 0.37460659341591196, 0.3907311284892735, 0.40673664307580054, 0.4226182617406996, 0.4383711467890774, 0.45399049973954664, 0.4694715627858904, 0.4848096202463365, 0.5000000000000001, 0.5150380749100542, 0.5299192642332047, 0.5446390350150266, 0.5591929034707462, 0.573576436351046, 0.5877852522924729, 0.6018150231520479, 0.6156614753256585, 0.6293203910498375, 0.6427876096865392, 0.656059028990507, 0.6691306063588578, 0.681998360062498, 0.6946583704589966, 0.7071067811865473, 0.7193398003386509, 0.7313537016191707, 0.7431448254773942, 0.7547095802227719, 0.7660444431189778, 0.7771459614569706, 0.7880107536067216, 0.7986355100472928, 0.8090169943749473, 0.8191520442889916, 0.8290375725550414, 0.838670567945424, 0.8480480961564254, 0.8571673007021121, 0.8660254037844384, 0.8746197071393958, 0.8829475928589269, 0.8910065241883678, 0.8987940462991671, 0.9063077870366497, 0.913545457642601, 0.9205048534524399, 0.9271838545667873, 0.9335804264972015, 0.9396926207859084, 0.9455185755993165, 0.9510565162951535, 0.9563047559630357, 0.9612616959383187, 0.9659258262890683, 0.9702957262759965, 0.9743700647852351, 0.9781476007338056, 0.981627183447664, 0.9848077530122079, 0.9876883405951377, 0.9902680687415702, 0.992546151641322, 0.9945218953682733, 0.9961946980917455, 0.9975640502598243, 0.9986295347545738, 0.9993908270190958, 0.9998476951563913
};

