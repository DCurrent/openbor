/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef UTILS_H
#define UTILS_H

// *** INCLUDES ***
#include <time.h>
#include "types.h"
#include "stringptr.h"

// *** TYPE DECLARATIONS ***
#define SCRIPT_LOG 0
#define OPENBOR_LOG 1
#define TIMESTAMP_PATTERN "%Y-%m-%d %H:%M:%S"

// *** VARIABLE DECLARATIONS ***
extern char debug_msg[2048];
extern u32 debug_time;

// *** FUNCTIONS DECLARATIONS ***
void writeToLogFile(const char *, ...);
void writeToScriptLog(const char *msg);
int fileExists(char *fnam);
int dirExists(char *dname, int create);
stringptr *readFromLogFile(int which);

void lc(char *buf, size_t size);
size_t getNewLineStart(char *buf);
void debugBuf(unsigned char *buf, size_t size, int columns);
void debug_printf(char *, ...);
void *checkAlloc(void *ptr, size_t size, const char *func, const char *file, int line);
void exitIfFalse(int value, const char *assertion, const char *func, const char *file, int line);
void abortIfFalse(int value, const char *assertion, const char *func, const char *file, int line);
void getPakName(char *name, int type);
void screenshot(s_screen *vscreen, unsigned char *pal, int ingame);
void getBasePath(char *newName, char *name, int type);
unsigned readlsb32(const unsigned char *src);
int searchList(const char *list[], const char *value, int length);
//int searchListB(const char *list[], const char *value, int length);
char *commaprint(u64 n);
char* multistrcatsp(char* buf, ...);
char* safe_strncpy(char* dst, const char* src, size_t size);
int safe_stricmp(const char *s1, const char *s2);
int safe_strnicmp(const char *s1, const char *s2, size_t n);
void get_time_string(char buffer[], unsigned buffer_size, time_t timestamp, char* pattern); // pattern ex. "%Y-%m-%d %H:%M:%S"
void get_now_string(char buffer[], unsigned buffer_size, char* pattern);

void Array_Check_Size( const char *f_caller, char **array, int new_size, int *curr_size_allocated, int grow_step );

#endif

