/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

/////////////////////////////////////////////////////////////////////////////

#ifndef GLOBALS_H
#define GLOBALS_H

/////////////////////////////////////////////////////////////////////////////

#include	<stdio.h>
#include    <assert.h>
#include    <inttypes.h>

#include	"utils.h"
#include	"tracemalloc.h"

#ifdef PSP
#include    <stdarg.h>
#include    <psppower.h>
#include    "pspport.h"
#include    "graphics.h"
#endif

#ifdef SDL
#include	"sdlport.h"
#endif

#ifdef GP2X
#include	"gp2xport.h"
#endif

#ifdef DOS
#include	"dosport.h"
#endif

#ifdef DC
#include	"dcport.h"
#endif

#ifdef XBOX
#include    "xboxport.h"
#endif

#ifdef WII
#include	<gctypes.h>
#include	<ogc/conf.h>
#include    "wiiport.h"
#endif

/////////////////////////////////////////////////////////////////////////////

#define printf writeToLogFile

/////////////////////////////////////////////////////////////////////////////

extern int int_assert[sizeof(int)==4?1:-1];

#endif
