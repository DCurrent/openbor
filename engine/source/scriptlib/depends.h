/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef DEPENDS_H
#define DEPENDS_H

#include "types.h"
#include "globals.h"

#ifndef COMPILED_SCRIPT
#define COMPILED_SCRIPT 1
#endif

typedef u32 DWORD;
typedef u32 ULONG;
typedef s32 LONG;

#ifndef WII
typedef int BOOL;
#endif

#ifndef XBOX
typedef short Wchar;
#endif

#ifdef VOID
#undef VOID
#endif
typedef void VOID;

#ifndef NULL
#define NULL 0
#endif

#ifndef XBOX
#ifdef S_OK
#undef S_OK
#endif
#define S_OK   ((ptrdiff_t)0)

#ifdef E_FAIL
#undef E_FAIL
#endif
#define E_FAIL ((ptrdiff_t)-1)

#ifdef FAILED
#undef FAILED
#endif
#define FAILED(status) (((ptrdiff_t)(status))<0)

#ifdef SUCCEEDED
#undef SUCCEEDED
#endif
#define SUCCEEDED(status) (((ptrdiff_t)(status))>=0)
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define MAX_STR_LEN    128
#define MAX_STR_VAR_LEN    64

#endif
