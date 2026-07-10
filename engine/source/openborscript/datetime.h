/*
* OpenBOR - http://www.chronocrash.com
* -----------------------------------------------------------------------
* All rights reserved. See LICENSE in OpenBOR root for license details.
*
* Copyright (c) 2004 OpenBOR Team
*/

#ifndef DATETIME_H
#define DATETIME_H 1

#include "ScriptVariant.h"

typedef enum e_datetime_standard {
    DATETIME_STANDARD_LOCAL,    // Local time, system wall clock
    DATETIME_STANDARD_UTC       // Coordinated Universal Time, system wall clock
} e_datetime_standard;

/*
* System wall-clock timestamp functions.
*/
HRESULT datetime_gettimestamp(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);    // Seconds since Unix epoch (1970-01-01 00:00:00 UTC)
HRESULT datetime_gettimestampms(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);  // Milliseconds since Unix epoch (1970-01-01 00:00:00 UTC)

/*
*
* System wall-clock format function.
*
* Accepts a strftime format string and returns formatted date/time text.
*
* datetime_format(format, milliseconds (optional), standard (optional))
*
* - Format: strftime format string, e.g. "%Y-%m-%d %H:%M:%S"
* - Milliseconds: Unix epoch milliseconds (optional, defaults to current system time if NULL or empty)
* - Standard: DATETIME_STANDARD_LOCAL or DATETIME_STANDARD_UTC (optional, defaults to local time standard if NULL or empty)
*/
HRESULT datetime_format(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);


/*
* System wall-clock component functions.
*
* These return integer values for gameplay logic.
*
* Optionaly accept a Unix epoch milliseconds argument and a time standard argument.
*
* Milliseconds argument behavior:
*   - no argument or NULL(): Use current system time, local standard.
*   - Integer:               Use provided Unix epoch milliseconds, local standard.
*
* Time standard values:
*
*   DATETIME_STANDARD_LOCAL
*   DATETIME_STANDARD_UTC
*/
HRESULT datetime_getyear(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT datetime_getmonth(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT datetime_getday(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT datetime_gethour(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT datetime_getminute(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);
HRESULT datetime_getsecond(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount);

#endif // DATETIME_H