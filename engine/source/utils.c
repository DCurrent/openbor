/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
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

#if PSP || WIN || LINUX || GP2X || DINGOO || SYMBIAN
#include <dirent.h>
#endif

#if GP2X || LINUX || DINGOO
#include <sys/stat.h>
#endif

#ifdef DOS
#include <direct.h>
#include "dosport.h"
#include "savepng.h"
#endif

#ifdef SDL
#include <unistd.h>
#include "sdlport.h"
#include "savepng.h"
#endif

#ifdef DC
#include "dcport.h"
#endif

#ifdef XBOX
#include "xboxport.h"
#include "savepng.h"
#endif

#ifdef PSP
#include "image.h"
#endif


#if WII && !SDL
#include "wiiport.h"
#include "savepng.h"
#endif

#if PSP || GP2X || LINUX || DINGOO || WII || SYMBIAN
#define MKDIR(x) mkdir(x, 0777)
#else
#define MKDIR(x) mkdir(x)
#endif

#ifdef XBOX
#define CHECK_LOGFILE(type)  type ? fileExists("d:\\Logs\\OpenBorLog.txt") : fileExists("d:\\Logs\\ScriptLog.txt")
#define OPEN_LOGFILE(type)   type ? fopen("d:\\Logs\\OpenBorLog.txt", "wt") : fopen("d:\\Logs\\ScriptLog.txt", "wt")
#define APPEND_LOGFILE(type) type ? fopen("d:\\Logs\\OpenBorLog.txt", "at") : fopen("d:\\Logs\\ScriptLog.txt", "at")
#define READ_LOGFILE(type)   type ? fopen("d:\\Logs\\OpenBorLog.txt", "rt") : fopen("d:\\Logs\\ScriptLog.txt", "rt")
#define COPY_ROOT_PATH(buf, name) strncpy(buf, "d:\\", 3); strncat(buf, name, strlen(name)); strncat(buf, "\\", 1)
#define COPY_PAKS_PATH(buf, name) strncpy(buf, "d:\\Paks\\", 8); strncat(buf, name, strlen(name))
#elif WII && !SDL
#define CHECK_LOGFILE(type)  type ? fileExists(getFullPath("Logs/OpenBorLog.txt")) : fileExists(getFullPath("Logs/ScriptLog.txt"))
#define OPEN_LOGFILE(type)   type ? fopen(getFullPath("Logs/OpenBorLog.txt"), "wt") : fopen(getFullPath("Logs/ScriptLog.txt"), "wt")
#define APPEND_LOGFILE(type) type ? fopen(getFullPath("Logs/OpenBorLog.txt"), "at") : fopen(getFullPath("Logs/ScriptLog.txt"), "at")
#define READ_LOGFILE(type)   type ? fopen(getFullPath("Logs/OpenBorLog.txt"), "rt") : fopen(getFullPath("Logs/ScriptLog.txt"), "rt")
#define COPY_ROOT_PATH(buf, name) strcpy(buf, rootDir); strncat(buf, name, strlen(name)); strncat(buf, "/", 1);
#define COPY_PAKS_PATH(buf, name) strncpy(buf, paksDir, strlen(paksDir)); strncat(buf, "/", 1); strncat(buf, name, strlen(name));
#elif WII && SDL
#define CHECK_LOGFILE(type)  type ? fileExists("sd:/apps/OpenBOR/Logs/OpenBorLog.txt") : fileExists("sd:/apps/OpenBOR/Logs/ScriptLog.txt")
#define OPEN_LOGFILE(type)   type ? fopen("sd:/apps/OpenBOR/Logs/OpenBorLog.txt", "wt") : fopen("sd:/apps/OpenBOR/Logs/ScriptLog.txt", "wt")
#define APPEND_LOGFILE(type) type ? fopen("sd:/apps/OpenBOR/Logs/OpenBorLog.txt", "at") : fopen("sd:/apps/OpenBOR/Logs/ScriptLog.txt", "at")
#define READ_LOGFILE(type)   type ? fopen("sd:/apps/OpenBOR/Logs/OpenBorLog.txt", "rt") : fopen("sd:/apps/OpenBOR/Logs/ScriptLog.txt", "rt")
#define COPY_ROOT_PATH(buf, name) strncpy(buf, "sd:/apps/OpenBOR/", 17); strncat(buf, name, strlen(name)); strncat(buf, "/", 1);
#define COPY_PAKS_PATH(buf, name) strncpy(buf, "sd:/apps/OpenBOR/Paks/", 22); strncat(buf, name, strlen(name));
#else
#define CHECK_LOGFILE(type)  type ? fileExists("./Logs/OpenBorLog.txt") : fileExists("./Logs/ScriptLog.txt")
#define OPEN_LOGFILE(type)   type ? fopen("./Logs/OpenBorLog.txt", "wt") : fopen("./Logs/ScriptLog.txt", "wt")
#define APPEND_LOGFILE(type) type ? fopen("./Logs/OpenBorLog.txt", "at") : fopen("./Logs/ScriptLog.txt", "at")
#define READ_LOGFILE(type)   type ? fopen("./Logs/OpenBorLog.txt", "rt") : fopen("./Logs/ScriptLog.txt", "rt")
#define COPY_ROOT_PATH(buf, name) strncpy(buf, "./", 2); strncat(buf, name, strlen(name)); strncat(buf, "/", 1);
#define COPY_PAKS_PATH(buf, name) strncpy(buf, "./Paks/", 7); strncat(buf, name, strlen(name));
#endif

void debugBuf(unsigned char* buf, size_t size, int columns) {
	size_t pos = 0;
	int i;
	while(pos<size) {
		for(i=0;i<columns;i++) {
			if(pos >= size) break;
			printf("%02x", buf[pos]);
			pos++;
		}
		printf("\n");
	}
}

//lowercases a buffer inplace
void lc(char* buf, size_t size) {
	ptrdiff_t i;
	for(i=0;i<size;i++)
		buf[i] = tolower((int)buf[i]);
}

// returns position after next newline in buf
size_t getNewLineStart(char* buf) {
	size_t res = 0;
	while(buf[res] && buf[res]!='\n' && buf[res]!='\r') ++res;
	while(buf[res] && (buf[res]=='\n' || buf[res]=='\r')) ++res;
	return res;
}

FILE* openborLog = NULL;
FILE* scriptLog = NULL;
char debug_msg[2048];
unsigned long debug_time = 0xFFFFFFFF;

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
#ifdef XBOX
	getBasePath(realName, dname, 0);
	return CreateDirectory(realName, NULL);
#else
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
		if(fd2 < 0) return 0;
#ifdef DARWIN
		chmod(realName, 0777);
#endif
		return 1;
	}
#endif
	return 0;
}

int fileExists(char *fnam)
{
	FILE *handle = NULL;
	if((handle=fopen(fnam,"rb")) == NULL) return 0;
	fclose(handle);
	return 1;
}

stringptr* readFromLogFile(int which)
{
	long  size;
	FILE* handle = NULL;
	stringptr* buffer = NULL;
	handle = READ_LOGFILE((which ? OPENBOR_LOG : SCRIPT_LOG));
	if(handle == NULL) return NULL;
	fseek(handle, 0, SEEK_END);
	size = ftell(handle);
	rewind(handle);
	if (size == 0) goto CLOSE_AND_QUIT;
	// allocate memory to contain the whole file:
	//buffer = (char*)malloc(sizeof(char)*(size+1)); // alloc one additional byte for the
	buffer = new_string(size);
	if(buffer == NULL) goto CLOSE_AND_QUIT;
	fread(buffer->ptr, 1, size, handle);
	CLOSE_AND_QUIT:
	fclose(handle);
	return buffer;
}
#endif

void writeToLogFile(const char * msg, ...)
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
		if(openborLog == NULL) return;
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
		if(scriptLog == NULL) return;
	}
	fwrite(msg, 1, strlen(msg), scriptLog);
	fflush(scriptLog);
#endif
}

void debug_printf(char *format, ...){
	va_list arglist;

	va_start(arglist, format);
	vsprintf(debug_msg, format, arglist);
	va_end(arglist);

	debug_time = 0xFFFFFFFF;
}

void getPakName(char name[256], int type){

	int i,x,y;
	char mod[256] = {""};

	strncpy(mod,packfile,strlen(packfile)-4);

	switch(type){
		case 0:
			strncat(mod,".sav",4);
			break;
		case 1:
			strncat(mod,".hi",3);
			break;
		case 2:
			strncat(mod,".scr",4);
			break;
		case 3:
			strncat(mod,".inp",4);
			break;
		case 4:
			strncat(mod,".cfg",4);
			break;
		default:
			// Loose extension!
			break;
	}

	x=0;
	for(i=0; i<(int)strlen(mod); i++){
		if((mod[i] == '/') || (mod[i] == '\\')) x = i;
	}
	y=0;
	for(i=0; i<(int)strlen(mod); i++){
		// For packfiles without '/'
		if(x == 0){
			name[y] = mod[i];
			y++;
		}
		// For packfiles with '/'
		if(x != 0 && i > x){
			name[y] = mod[i];
			y++;
		}
	}
}

void screenshot(s_screen *vscreen, unsigned char *pal, int ingame){
#ifndef DC
	int	 shotnum = 0;
	char shotname[128] = {""};
	char modname[128]  = {""};

	getPakName(modname,99);
#ifdef PSP
	if(dirExists("ms0:/PICTURE/", 1) && dirExists("ms0:/PICTURE/Beats Of Rage/", 1)){
#endif
		do{
#if PSP
			sprintf(shotname, "ms0:/PICTURE/Beats Of Rage/%s - ScreenShot - %02u.png", modname,shotnum);
#elif DOS
			sprintf(shotname, "./SShots/s%04u.png", shotnum);
#elif XBOX
			sprintf(shotname, "d:\\ScreenShots\\%s - %04u.png", modname,shotnum);
#elif WII && SDL
			sprintf(shotname, "sd:/apps/OpenBOR/ScreenShots/%s - %04u.png", modname,shotnum);
#elif WII
			sprintf(shotname, "%s/%s - %04u.png", screenShotsDir,modname,shotnum);
#else
			sprintf(shotname, "./ScreenShots/%s - %04u.png", modname,shotnum);
#endif
			++shotnum;
		}while(fileExists(shotname) && shotnum<10000);

#ifdef PSP
		if(shotnum<10000) saveImage(shotname);
#else
		if(shotnum<10000) savepng(shotname, vscreen, pal);
#endif
		if(ingame) debug_printf("Saved %s", shotname);
#ifdef PSP
	}
#endif
#endif
}

#ifdef XBOX
int findmods(void)
{
	int i = 0;
	HANDLE hFind;
	WIN32_FIND_DATAA oFindData;
	hFind = FindFirstFile("d:\\Paks\\*", &oFindData);
	if(hFind == INVALID_HANDLE_VALUE) return 1;
	do
	{
		if(stristr(oFindData.cFileName, ".pak") && stricmp(oFindData.cFileName, "menu.pak") != 0)
		{
			strncpy(paklist[i].filename, oFindData.cFileName, 128-strlen(oFindData.cFileName));
			i++;
		}
	}
	while(FindNextFile(hFind, &oFindData));
	FindClose(hFind);
	return i;
}
#endif

unsigned readlsb32(const unsigned char *src)
{
	return
		((((unsigned)(src[0])) & 0xFF) <<  0) |
		((((unsigned)(src[1])) & 0xFF) <<  8) |
		((((unsigned)(src[2])) & 0xFF) << 16) |
		((((unsigned)(src[3])) & 0xFF) << 24);
}

// Optimized search in an arranged string table, return the index
int searchList(const char* list[], const char* value, int length)
{
	int i;
	int a = 0;
	int b = length / 2;
	int c = length - 1;
	int v = value[0];

	// We must convert uppercase values to lowercase,
	// since this is how every command is written in
	// our source.  Refer to an ASCII Chart
	if(v >= 0x41 && v <= 0x5A) v += 0x20;

	// Index value equals middle value,
	// Lets search starting from center.
	if(v == list[b][0])
	{
		if(stricmp(list[b], value) == 0) return b;

		// Search Down the List.
		if(v == list[b-1][0])
		{
			for(i=b-1 ; i>=0; i--)
			{
				if(stricmp(list[i], value) == 0) return i;
				if(v != list[i-1][0]) break;
			}
		}

		// Search Up the List.
		if(v == list[b+1][0])
		{
			for(i=b+1; i<length; i++)
			{
				if(stricmp(list[i], value) == 0) return i;
				if(v != list[i+1][0]) break;
			}
		}

		// No match, return failure.
		goto searchListFailed;
	}

	// Define the starting point.
	if(v >= list[b+1][0]) a = b+1;
	else if(v <= list[b-1][0]) c = b-1;
	else goto searchListFailed;

	// Search Up from starting point.
	for(i=a; i<=c; i++)
	{
		if(v == list[i][0])
		{
			if(stricmp(list[i], value) == 0) return i;
			if(v != list[i+1][0]) break;
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
	char *p = &retbuf[sizeof(retbuf)-1];
	int i = 0;

	if(comma == '\0')
	{
		struct lconv *lcp = localeconv();
		if(lcp != NULL)
		{
			if(lcp->thousands_sep != NULL &&
			   *lcp->thousands_sep != '\0')
				comma = *lcp->thousands_sep;
			else
				comma = ',';
		}
	}

	*p = '\0';

	do {
		if(i%3 == 0 && i != 0)
			*--p = comma;
		*--p = '0' + n % 10;
		n /= 10;
		i++;
	} while(n != 0);

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
Array_Check_Size( const char* f_caller, char** array, int new_size, int* curr_size_allocated, int grow_step )
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
			shutdown(1, "Out Of Memory!  Failed in %s\n", f_caller);
		memset( *array, 0, *curr_size_allocated );
		return;
	}

	// No need to decrease or increase the array
	else if( new_size > (*curr_size_allocated - grow_step ) && new_size <= *curr_size_allocated )
		return;

	//-------------------------------------------
	// Must increase or decrease the array size

	int old_size = *curr_size_allocated;

	// Recompute needed size
	*curr_size_allocated = ((int)ceil((float)new_size / (float)grow_step)) * grow_step;

	// Alloc a new array
	void* copy = malloc(*curr_size_allocated );
	if(copy == NULL)
		shutdown(1, "Out Of Memory!  Failed in %s\n", f_caller);

	// Copy the previous content of the array
	memcpy(copy, *array, ( (old_size<new_size) ?old_size :new_size) );

	// Init the new allocations
	if( old_size < *curr_size_allocated )
		memset( copy + old_size, 0, *curr_size_allocated - old_size );

	// Free previous array memory
	free(*array);

	// ReAssign the new allocated array
	*array = copy;
}

