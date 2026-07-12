/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved. See LICENSE in OpenBOR root for license details.
 *
 * Copyright (c) 2004 OpenBOR Team
 */

#include "datetime.h"
#include "ScriptVariant.h"
#include "ScriptVariantInteger.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

/*
* Conditionally include platform-specific 
* headers for time functions. We can't
* include windows.h because it conflicts with
* other headers in the OpenBOR engine.
*/

#if defined(_WIN32)
typedef struct s_datetime_windows_filetime {
    uint32_t dwLowDateTime;
    uint32_t dwHighDateTime;
} s_datetime_windows_filetime;

#if defined(_MSC_VER)
#define DATETIME_WINDOWS_IMPORT __declspec(dllimport)
#define DATETIME_WINDOWS_CALL   __stdcall
#elif defined(__GNUC__) && defined(__i386__)
#define DATETIME_WINDOWS_IMPORT __declspec(dllimport)
#define DATETIME_WINDOWS_CALL   __attribute__((stdcall))
#else
#define DATETIME_WINDOWS_IMPORT __declspec(dllimport)
#define DATETIME_WINDOWS_CALL
#endif

DATETIME_WINDOWS_IMPORT void DATETIME_WINDOWS_CALL GetSystemTimeAsFileTime(s_datetime_windows_filetime *file_time);

#elif defined(__linux__) || defined(__ANDROID__) || defined(__APPLE__)
#include <sys/time.h>
#else
#error datetime.c currently supports Windows, Linux, macOS, and Android.
#endif

#define DATETIME_FORMAT_BUFFER_SIZE 256

/*
* Windows FILETIME epoch:
*
* FILETIME counts 100-nanosecond intervals since 
* 1601-01-01 UTC. Just have to be different, don't
* you, Windows?
*
* Unix time counts seconds since 1970-01-01 00:00:00 UTC.
*
* This constant is the number of 100-nanosecond 
* intervals between those two epochs.
*/
#if defined(_WIN32)
#define DATETIME_WINDOWS_TO_UNIX_EPOCH_100NS 116444736000000000ULL
#define DATETIME_100NS_PER_MILLISECOND       10000ULL
#endif

/*
* Forward declarations for internal support functions.
*/
static HRESULT datetime_get_epoch_milliseconds(uint64_t *milliseconds);
static HRESULT datetime_resolve_time(ScriptVariant **varlist, const int paramCount, uint64_t *milliseconds_result, struct tm *time_result);
static HRESULT datetime_set_formatted_string(ScriptVariant **varlist, ScriptVariant *pretvar, const int paramCount, const char *format);

/*
* Caskey, Damon V.
* 2026-07-09
*
* Get Unix epoch milliseconds from the system wall clock.
*/
static HRESULT datetime_get_epoch_milliseconds(uint64_t *milliseconds) {
    if(!milliseconds) {
        return E_FAIL;
    }

#if defined(_WIN32)
    {
        s_datetime_windows_filetime file_time;
        uint64_t windows_epoch_100ns;
        uint64_t unix_epoch_100ns;

        GetSystemTimeAsFileTime(&file_time);

        /*
        * Windows FILETIME stores the timestamp as 
        * two 32-bit chunks. Since Windows already 
        * made us do epoch math, naturally it also 
        * makes us put the number back together.
        */
        windows_epoch_100ns = ((uint64_t)file_time.dwHighDateTime << 32) |
                              (uint64_t)file_time.dwLowDateTime;

        unix_epoch_100ns = windows_epoch_100ns - DATETIME_WINDOWS_TO_UNIX_EPOCH_100NS;

        *milliseconds = unix_epoch_100ns / DATETIME_100NS_PER_MILLISECOND;

        return S_OK;
    }
#else
    {
        struct timeval time_value;

        if(gettimeofday(&time_value, NULL) != 0) {
            return E_FAIL;
        }

        *milliseconds = ((uint64_t)time_value.tv_sec * 1000ULL) +
                        ((uint64_t)time_value.tv_usec / 1000ULL);

        return S_OK;
    }
#endif
}

/*
* Caskey, Damon V.
* 2026-07-09
*
* Resolve date/time API arguments.
*
* Supported signatures:
*
*   datetime_format()                       - Use current Unix epoch milliseconds and local time.
*   datetime_format(milliseconds)           - Use provided Unix epoch milliseconds and local time.
*   datetime_format(milliseconds, standard) - Use provided/current Unix epoch milliseconds and requested time standard.
*
* NULL/empty milliseconds are treated as omitted so callers can request
* current UTC without providing their own timestamp:
*
*   datetime_format(NULL(), DATETIME_STANDARD_UTC)
*
* The broken-down time result is optional. Callers that only need raw
* milliseconds can pass NULL for time_result.
*/
static HRESULT datetime_resolve_time(ScriptVariant **varlist, const int paramCount, uint64_t *milliseconds_result, struct tm *time_result) {
    uint64_t milliseconds;
    uint64_t standard_value;
    e_datetime_standard standard;
    uint64_t seconds;
    time_t raw_time;

#if !defined(_WIN32)
    struct tm *converted_time;
#endif

    milliseconds = 0;
    standard_value = DATETIME_STANDARD_LOCAL;
    standard = DATETIME_STANDARD_LOCAL;

    if(paramCount < 0 || paramCount > 2) {
        printf("\ndatetime_resolve_time(): Invalid parameter count: %d.\n"
               "Check your time function input: datetime_format(format, milliseconds, standard).\n\n",
               paramCount);

        return E_FAIL;
    }

    /*
    * Resolve milliseconds.
    *
    * NULL/empty milliseconds means "use current time".
    * This lets callers request current UTC with:
    *
    *   datetime_format(format, NULL(), DATETIME_STANDARD_UTC)
    */
    if(paramCount == 0 ||
       !varlist ||
       !varlist[0] ||
       varlist[0]->vt == VT_EMPTY ||
       (varlist[0]->vt == VT_PTR && !varlist[0]->ptrVal)) {

        /*
        * Default to current system time if no milliseconds 
        * argument is provided.
        */
        if(datetime_get_epoch_milliseconds(&milliseconds) != S_OK) {
            printf("\ndatetime_resolve_time(): Failed to get current system time.\n"
                   "Check platform clock support.\n\n");

            return E_FAIL;
        }

    } else if(ScriptVariant_Unsigned64Value(varlist[0], &milliseconds) != S_OK) {

        printf("\ndatetime_resolve_time(): Invalid milliseconds argument.\n"
               "Expected unsigned integer milliseconds or NULL.\n"
               "Check your time function input: datetime_format(format, milliseconds, standard).\n\n");

        return E_FAIL;
    }

    /*
    * Resolve optional time standard.
    */
    if(paramCount == 2) {
        if(!varlist ||
            !varlist[1] ||
            varlist[1]->vt == VT_EMPTY ||
            (varlist[1]->vt == VT_PTR && !varlist[1]->ptrVal)) {

            /*
            * Default to local time if no time standard
            * argument is provided.
            */
            standard = DATETIME_STANDARD_LOCAL;

        } else if(ScriptVariant_Unsigned64Value(varlist[1], &standard_value) != S_OK) {
            printf("\ndatetime_resolve_time(%llu): Invalid time standard argument.\n"
                "Expected openborconstant(\"DATETIME_STANDARD_LOCAL\"), openborconstant(\"DATETIME_STANDARD_UTC\"), or NULL.\n"
                "Check your time function input: datetime_format(format, milliseconds, standard).\n\n",
                (unsigned long long)milliseconds);

            return E_FAIL;

        } else {
            /*
            * Validate time standard value. Pass it on
            * if valid, otherwise report error.
            */
            switch(standard_value) {
                case DATETIME_STANDARD_LOCAL:
                    standard = DATETIME_STANDARD_LOCAL;
                    break;

                case DATETIME_STANDARD_UTC:
                    standard = DATETIME_STANDARD_UTC;
                    break;

                default:
                    printf("\ndatetime_resolve_time(%llu, %llu): Invalid time standard value: %llu.\n"
                        "Expected openborconstant(\"DATETIME_STANDARD_LOCAL\") or openborconstant(\"DATETIME_STANDARD_UTC\").\n"
                        "Check your time function input: datetime_format(format,milliseconds, standard).\n\n",
                        (unsigned long long)milliseconds,
                        (unsigned long long)standard_value,
                        (unsigned long long)standard_value);

                    return E_FAIL;
            }
        }
    }

    if(milliseconds_result) {
        *milliseconds_result = milliseconds;
    }

    /*
    * Raw millisecond callers are done here.
    */
    if(!time_result) {
        return S_OK;
    }

    /*
    * Convert Unix epoch milliseconds to broken-down time.
    */
    seconds = milliseconds / 1000ULL;
    raw_time = (time_t)seconds;

    /*
    * Catch obvious time_t narrowing.
    */
    if((uint64_t)raw_time != seconds) {
        printf("\ndatetime_resolve_time(%llu, %llu): Time value exceeds platform time_t range.\n"
               "Check your time function input: datetime_format(format, milliseconds, standard).\n\n",
               (unsigned long long)milliseconds,
               (unsigned long long)standard);

        return E_FAIL;
    }

#if defined(_WIN32)

    /*
    * Since Windows has to be weird, let's 
    * use the secure versions of gmtime and 
    * localtime. 
    */

    if(standard == DATETIME_STANDARD_UTC) {
        if(gmtime_s(time_result, &raw_time) != 0) {
            printf("\ndatetime_resolve_time(%llu, %llu): Failed to convert time to UTC.\n\n",
                   (unsigned long long)milliseconds,
                   (unsigned long long)standard);

            return E_FAIL;
        }

        return S_OK;
    }

    if(localtime_s(time_result, &raw_time) != 0) {
        printf("\ndatetime_resolve_time(%llu, %llu): Failed to convert time to local time.\n\n",
               (unsigned long long)milliseconds,
               (unsigned long long)standard);

        return E_FAIL;
    }

    return S_OK;

#else

    if(standard == DATETIME_STANDARD_UTC) {
        converted_time = gmtime_r(&raw_time, time_result);
    } else {
        converted_time = localtime_r(&raw_time, time_result);
    }

    if(!converted_time) {
        printf("\ndatetime_resolve_time(%llu, %llu): Failed to convert time.\n\n",
               (unsigned long long)milliseconds,
               (unsigned long long)standard);

        return E_FAIL;
    }

    return S_OK;

#endif
}

/*
* Caskey, Damon V.
* 2026-07-09
*
* Format time from either current clock or 
* caller-provided epoch milliseconds.
*/
static HRESULT datetime_set_formatted_string(ScriptVariant **varlist, ScriptVariant *pretvar, const int paramCount, const char *format) {
    struct tm time_value;
    char buffer[DATETIME_FORMAT_BUFFER_SIZE];

    if(!pretvar) {
        printf("\ndatetime_set_formatted_string(): Missing return variant.\n\n");
        return E_FAIL;
    }

    if(!format) {
        printf("\ndatetime_set_formatted_string(): Missing time format.\n\n");
        return E_FAIL;
    }

    /*
    * Get time value according to provided arguments.
    */
    if(datetime_resolve_time(varlist, paramCount, NULL, &time_value) != S_OK) {
        return E_FAIL;
    }

    if(!strftime(buffer, sizeof(buffer), format, &time_value)) {
        printf("\ndatetime_set_formatted_string(): Failed to format time with format: %s.\n\n",
               format);

        return E_FAIL;
    }

    ScriptVariant_ChangeType(pretvar, VT_STR);
    pretvar->strVal = StrCache_CreateNewFrom(buffer);

    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-07-09
*
* Return current Unix epoch seconds.
*/
HRESULT datetime_gettimestamp(ScriptVariant **varlist, ScriptVariant **pretvar, const int paramCount) {
    uint64_t milliseconds;

    (void)varlist;

    if(!pretvar || !*pretvar) {
        printf("\ndatetime_gettimestamp(): Missing return variant.\n\n");
        return E_FAIL;
    }

    if(paramCount != 0) {
        printf("\ndatetime_gettimestamp(): Invalid parameter count: %d.\n"
               "This function does not accept arguments.\n\n",
               paramCount);

        *pretvar = NULL;
        return E_FAIL;
    }

    if(datetime_get_epoch_milliseconds(&milliseconds) != S_OK) {
        printf("\ndatetime_gettimestamp(): Failed to get current system time.\n"
               "Check platform clock support.\n\n");

        *pretvar = NULL;
        return E_FAIL;
    }

    return ScriptVariant_SetUnsignedIntegerResult(*pretvar, milliseconds / 1000ULL, 1);
}

/*
* Caskey, Damon V.
* 2026-07-09
*
* Return current Unix epoch milliseconds.
*/
HRESULT datetime_gettimestampms(ScriptVariant **varlist, ScriptVariant **pretvar, const int paramCount) {
    uint64_t milliseconds;

    (void)varlist;

    if(!pretvar || !*pretvar) {
        printf("\ndatetime_gettimestampms(): Missing return variant.\n\n");
        return E_FAIL;
    }

    if(paramCount != 0) {
        printf("\ndatetime_gettimestampms(): Invalid parameter count: %d.\n"
               "This function does not accept arguments.\n\n",
               paramCount);

        *pretvar = NULL;
        return E_FAIL;
    }

    if(datetime_get_epoch_milliseconds(&milliseconds) != S_OK) {
        printf("\ndatetime_gettimestampms(): Failed to get current system time.\n"
               "Check platform clock support.\n\n");

        *pretvar = NULL;
        return E_FAIL;
    }

    return ScriptVariant_SetUnsignedIntegerResult(*pretvar, milliseconds, 1);
}

/*
* Caskey, Damon V.
* 2026-07-09
*
* datetime_format(format[, milliseconds][, standard])
*
* Return date/time using caller-provided strftime format.
*
* Format examples:
*
*   "%Y-%m-%d" - Return date as YYYY-MM-DD.
*   "%H:%M:%S" - Return time as HH:MM:SS.
*   "%Y-%m-%d %H:%M:%S" - Return date and time as YYYY-MM-DD HH:MM:SS.
*   "%A, %B %d, %Y" - Return date as "Weekday, Month Day, Year".
*/
HRESULT datetime_format(ScriptVariant **varlist, ScriptVariant **pretvar, const int paramCount) {
    const char *format;

    if(!pretvar || !*pretvar) {
        printf("\ndatetime_format(): Missing return variant.\n\n");
        return E_FAIL;
    }

    if(paramCount < 1 || paramCount > 3) {
        printf("\ndatetime_format(): Invalid parameter count: %d.\n"
               "Check your time function input: datetime_format(format, milliseconds, standard).\n\n",
               paramCount);

        *pretvar = NULL;
        return E_FAIL;
    }

    if(!varlist || !varlist[0] || varlist[0]->vt != VT_STR || varlist[0]->strVal < 0) {
        printf("\ndatetime_format(): Invalid format argument.\n"
            "Expected format string.\n"
            "Check your time function input: datetime_format(format, milliseconds, standard).\n\n");

        *pretvar = NULL;
        return E_FAIL;
    }

    format = StrCache_Get(varlist[0]->strVal);

    if(!format || !format[0]) {
        printf("\ndatetime_format(): Empty format string.\n"
               "Expected a valid strftime format string.\n\n");

        *pretvar = NULL;
        return E_FAIL;
    }

    /*
    * Reuse the existing time resolver by skipping the format
    * argument. That gives us:
    *
    *   datetime_format(format)
    *   datetime_format(format, milliseconds)
    *   datetime_format(format, milliseconds, standard)
    */
    if(datetime_set_formatted_string(varlist + 1, *pretvar, paramCount - 1, format) != S_OK) {
        *pretvar = NULL;
        return E_FAIL;
    }

    return S_OK;
}

/*
* Caskey, Damon V.
* 2026-07-09
*
* datetime_getyear([milliseconds][, standard])
*
* Return year as an integer.
*/
HRESULT datetime_getyear(ScriptVariant **varlist, ScriptVariant **pretvar, const int paramCount) {
    struct tm time_value;

    if(!pretvar || !*pretvar) {
        printf("\ndatetime_getyear(): Missing return variant.\n\n");
        return E_FAIL;
    }

    if(datetime_resolve_time(varlist, paramCount, NULL, &time_value) != S_OK) {
        *pretvar = NULL;
        return E_FAIL;
    }

    return ScriptVariant_SetUnsignedIntegerResult(*pretvar, (uint64_t)(time_value.tm_year + 1900), 1);
}

/*
* Caskey, Damon V.
* 2026-07-09
*
* datetime_getmonth([milliseconds][, standard])
*
* Return month as an integer, 1-12.
*/
HRESULT datetime_getmonth(ScriptVariant **varlist, ScriptVariant **pretvar, const int paramCount) {
    struct tm time_value;

    if(!pretvar || !*pretvar) {
        printf("\ndatetime_getmonth(): Missing return variant.\n\n");
        return E_FAIL;
    }

    if(datetime_resolve_time(varlist, paramCount, NULL, &time_value) != S_OK) {
        *pretvar = NULL;
        return E_FAIL;
    }

    return ScriptVariant_SetUnsignedIntegerResult(*pretvar, (uint64_t)(time_value.tm_mon + 1), 1);
}

/*
* Caskey, Damon V.
* 2026-07-09
*
* datetime_getday([milliseconds][, standard])
*
* Return day of month as an integer, 1-31.
*/
HRESULT datetime_getday(ScriptVariant **varlist, ScriptVariant **pretvar, const int paramCount) {
    struct tm time_value;

    if(!pretvar || !*pretvar) {
        printf("\ndatetime_getday(): Missing return variant.\n\n");
        return E_FAIL;
    }

    if(datetime_resolve_time(varlist, paramCount, NULL, &time_value) != S_OK) {
        *pretvar = NULL;
        return E_FAIL;
    }

    return ScriptVariant_SetUnsignedIntegerResult(*pretvar, (uint64_t)time_value.tm_mday, 1);
}

/*
* Caskey, Damon V.
* 2026-07-12
*
* datetime_getweekday([milliseconds][, standard])
*
* Return weekday as an integer, 0-6.
*/
HRESULT datetime_getweekday(ScriptVariant **varlist, ScriptVariant **pretvar, int paramCount) {
    struct tm time_value;
    e_datetime_weekday weekday;

    if(!pretvar || !*pretvar) {
        printf("\ndatetime_getweekday(): Missing return variant.\n\n");
        return E_FAIL;
    }

    if(datetime_resolve_time(varlist, paramCount, NULL, &time_value) != S_OK) {
        *pretvar = NULL;
        return E_FAIL;
    }

    weekday = (e_datetime_weekday)time_value.tm_wday;

    return ScriptVariant_SetUnsignedIntegerResult(*pretvar, (uint64_t)weekday, 1);
}

/*
* Caskey, Damon V.
* 2026-07-09
*
* datetime_gethour([milliseconds][, standard])
*
* Return hour as an integer, 0-23.
*/
HRESULT datetime_gethour(ScriptVariant **varlist, ScriptVariant **pretvar, const int paramCount) {
    struct tm time_value;

    if(!pretvar || !*pretvar) {
        printf("\ndatetime_gethour(): Missing return variant.\n\n");
        return E_FAIL;
    }

    if(datetime_resolve_time(varlist, paramCount, NULL, &time_value) != S_OK) {
        *pretvar = NULL;
        return E_FAIL;
    }

    return ScriptVariant_SetUnsignedIntegerResult(*pretvar, (uint64_t)time_value.tm_hour, 1);
}

/*
* Caskey, Damon V.
* 2026-07-09
*
* datetime_getminute([milliseconds][, standard])
*
* Return minute as an integer, 0-59.
*/
HRESULT datetime_getminute(ScriptVariant **varlist, ScriptVariant **pretvar, const int paramCount) {
    struct tm time_value;

    if(!pretvar || !*pretvar) {
        printf("\ndatetime_getminute(): Missing return variant.\n\n");
        return E_FAIL;
    }

    if(datetime_resolve_time(varlist, paramCount, NULL, &time_value) != S_OK) {
        *pretvar = NULL;
        return E_FAIL;
    }

    return ScriptVariant_SetUnsignedIntegerResult(*pretvar, (uint64_t)time_value.tm_min, 1);
}

/*
* Caskey, Damon V.
* 2026-07-09
*
* datetime_getsecond([milliseconds][, standard])
*
* Return second as an integer, 0-60.
*
* Some C runtimes may report 60 for leap-second representation.
*/
HRESULT datetime_getsecond(ScriptVariant **varlist, ScriptVariant **pretvar, const int paramCount) {
    struct tm time_value;

    if(!pretvar || !*pretvar) {
        printf("\ndatetime_getsecond(): Missing return variant.\n\n");
        return E_FAIL;
    }

    if(datetime_resolve_time(varlist, paramCount, NULL, &time_value) != S_OK) {
        *pretvar = NULL;
        return E_FAIL;
    }

    return ScriptVariant_SetUnsignedIntegerResult(*pretvar, (uint64_t)time_value.tm_sec, 1);
}

#if defined(_WIN32)
#undef DATETIME_WINDOWS_IMPORT
#undef DATETIME_WINDOWS_CALL
#endif

#undef DATETIME_FORMAT_BUFFER_SIZE