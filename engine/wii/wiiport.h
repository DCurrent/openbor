/*
 * OpenBOR - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef _WIIPORT_H_
#define _WIIPORT_H_

#include <stdarg.h>

#include "globals.h"

//#define stricmp safe_stricmp
//#define strnicmp safe_strnicmp

char* getFullPath(char *relPath);
void borExit(int reset);
void openborMain(int argc, char** argv);

extern char packfile[MAX_FILENAME_LEN];
extern char paksDir[MAX_FILENAME_LEN];
extern char savesDir[MAX_FILENAME_LEN];
extern char logsDir[MAX_FILENAME_LEN];
extern char screenShotsDir[MAX_FILENAME_LEN];
extern char rootDir[MAX_FILENAME_LEN];

#endif
