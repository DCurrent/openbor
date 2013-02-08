/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2013 OpenBOR Team
 */

#ifndef UTILS_H
#define UTILS_H

// *** INCLUDES ***
#include "types.h"
#include "stringptr.h"

// *** TYPE DECLARATIONS ***
#define SCRIPT_LOG 0
#define OPENBOR_LOG 1

// *** VARIABLE DECLARATIONS ***
extern char debug_msg[2048];
extern u32 debug_time;

// *** FUNCTIONS DECLARATIONS ***
void writeToLogFile(const char *, ...);
void writeToScriptLog(const char *msg);

#ifndef DC
int fileExists(char *fnam);
int dirExists(char *dname, int create);
stringptr* readFromLogFile(int which);
#endif

#if XBOX || DC
typedef struct{
	char filename[80];
}s_filelist;
s_filelist paklist[20];

int findmods(void);
#endif

void lc(char* buf, size_t size);
size_t getNewLineStart(char* buf);
void debugBuf(unsigned char* buf, size_t size, int columns);
void debug_printf(char *, ...);
void exitIfFalse(int value, const char* assertion, const char* func, const char* file, int line);
void abortIfFalse(int value, const char* assertion, const char* func, const char* file, int line);
void getPakName(char* name, int type);
void screenshot(s_screen *vscreen, unsigned char *pal, int ingame);
void getBasePath(char *newName, char *name, int type);
unsigned readlsb32(const unsigned char *src);
int searchList(const char* list[], const char* value, int length);
char *commaprint(u64 n);

void Array_Check_Size( const char* f_caller, char** array, int new_size, int* curr_size_allocated, int grow_step );

#endif

