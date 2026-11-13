/* Ordinary engine logging must remain active when profiling is disabled. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpu.h"
#include "engine_profile.h"
#include "utils.h"

static char ordinary_log[16384];
static unsigned ordinary_log_calls;

/*
 * Damon V. Caskey
 * 2026-11-13
 * Record the production log boundary without linking the game filesystem.
 */
void writeToLogFile(const char *format, ...)
{
    va_list arguments;
    int length;

    /* The real formatter must deliver a complete record through this sink. */
    va_start(arguments, format);
    length = vsnprintf(ordinary_log, sizeof(ordinary_log), format, arguments);
    va_end(arguments);
    assert(length >= 0 && (size_t)length < sizeof(ordinary_log));
    ++ordinary_log_calls;
}

/*
 * Damon V. Caskey
 * 2026-11-13
 * Exercise gpu_log without PP_TEST so a profiler-only implementation fails.
 */
int main(void)
{
    char driver_diagnostic[8192];
    size_t length, index;

    assert(setenv("OPENBOR_PROFILE", "0", 1) == 0);
    assert(!engine_profile_enabled());
    engine_profile_log("This profiling record must remain disabled.");
    assert(ordinary_log_calls == 0);

    /* Driver text containing format directives remains data at both stages. */
    gpu_log("device=%s units=%u", "test GPU %n %s", 5u);
    assert(ordinary_log_calls == 1);
    assert(!strcmp(ordinary_log, "[GPU] device=test GPU %n %s units=5\n"));

    /* Long compiler diagnostics are bounded while preserving the prefix and
     * newline. The contract does not depend on the exact private buffer size. */
    memset(driver_diagnostic, 'x', sizeof(driver_diagnostic) - 1);
    driver_diagnostic[sizeof(driver_diagnostic) - 1] = '\0';
    gpu_log("%s", driver_diagnostic);
    assert(ordinary_log_calls == 2);
    length = strlen(ordinary_log);
    assert(length > 7 && length < sizeof(driver_diagnostic));
    assert(!strncmp(ordinary_log, "[GPU] ", 6));
    assert(ordinary_log[length - 1] == '\n');
    for(index = 6; index + 1 < length; ++index) assert(ordinary_log[index] == 'x');
    gpu_log(NULL);
    assert(ordinary_log_calls == 2 && strlen(ordinary_log) == length);

    puts("GPU ordinary log tests passed with profiling disabled.");
    return 0;
}
