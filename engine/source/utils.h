/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef UTILS_H
#define UTILS_H

// *** INCLUDES ***
#include "types.h"

// *** TYPE DECLARATIONS ***
#define SCRIPT_LOG 0
#define OPENBOR_LOG 1

// *** VARIABLE DECLARATIONS ***
extern char debug_msg[2048];
extern unsigned long debug_time;

// *** FUNCTIONS DECLARATIONS ***
void writeToLogFile(const char *, ...);
void writeToScriptLog(const char *msg);

#ifndef DC
int fileExists(char *fnam);
int dirExists(char *dname, int create);
char* readFromLogFile(int which);
#endif

#if XBOX || DC
typedef struct{
	char filename[80];
}s_filelist;
s_filelist paklist[20];

int findmods(void);
#endif

void debug_printf(char *, ...);
void getPakName(char name[256], int type);
void screenshot(s_screen *vscreen, unsigned char *pal, int ingame);
void getBasePath(char *newName, char *name, int type);
unsigned readlsb32(const unsigned char *src);
int searchList(const char* list[], const char* value, int length);
char *commaprint(u64 n);

void Array_Check_Size( const char* f_caller, char** array, int new_size, int* curr_size_allocated, int grow_step );

#endif

