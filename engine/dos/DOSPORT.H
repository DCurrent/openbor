#ifndef DOSPORT_H
#define DOSPORT_H

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <direct.h>
#include "joy.h"
#include "keyboard.h"
#include "globals.h"


// *** VARIABLE DECLARATIONS ***
extern char packfile[128];


// *** FUNCTION DECLARATIONS ***
void borExit(int reset);
void openborMain(void);

#endif
