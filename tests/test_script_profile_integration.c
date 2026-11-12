/*
 * Real parser/interpreter coverage for engine-side script profiling.
 *
 * Native fixture functions replace game services, but parsing, compilation,
 * imports, argument/return handling and bytecode execution use production code.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "engine_profile.h"
#include "Interpreter.h"
#include "ImportCache.h"

static uint64_t fixture_time;
static uint64_t fixture_clock_reads;
static unsigned fixture_work_calls;
static int fixture_work_sum;
static unsigned fixture_outer_calls;
static unsigned fixture_error_calls;
static char fixture_log[65536];
static size_t fixture_log_used;
static List fixture_functions;
static Interpreter fixture_child;
static int fixture_scene;
static int fixture_target;
static int fixture_import_position;
static int fixture_import_open;
static const char fixture_import_source[] =
    "void imported_work() { profile_work(7); }\n";

/* Game-global archive selection is unnecessary for the in-memory fixture. */
char packfile[MAX_FILENAME_LEN];

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Retain production safe-allocation checks without linking unrelated services.
 */
void *checkAlloc(void *pointer, size_t size, const char *function,
    const char *file, int line)
{
    (void)size;
    (void)function;
    (void)file;
    (void)line;
    assert(pointer);
    return pointer;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Resolve exactly one fixture import; optional game headers remain absent.
 */
int openpackfile(const char *filename, const char *archive)
{
    (void)archive;
    if(strcmp(filename, "profiling_fixture_import.c") != 0)
    {
        return -1;
    }
    assert(!fixture_import_open);
    fixture_import_open = 1;
    fixture_import_position = 0;
    return 1;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Read the fixture import through the same interface used by ImportCache.
 */
int readpackfile(int handle, void *buffer, int length)
{
    int available = (int)strlen(fixture_import_source) - fixture_import_position;

    assert(handle == 1 && fixture_import_open);
    if(length > available)
    {
        length = available;
    }
    assert(length >= 0);
    memcpy(buffer, fixture_import_source + fixture_import_position, (size_t)length);
    fixture_import_position += length;
    return length;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Seek only within the in-memory import, rejecting unrelated handles.
 */
int seekpackfile(int handle, int offset, int origin)
{
    int position = offset;

    assert(handle == 1 && fixture_import_open);
    if(origin == SEEK_CUR)
    {
        position += fixture_import_position;
    }
    else if(origin == SEEK_END)
    {
        position += (int)strlen(fixture_import_source);
    }
    else
    {
        assert(origin == SEEK_SET);
    }
    assert(position >= 0 && position <= (int)strlen(fixture_import_source));
    fixture_import_position = position;
    return position;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Close the fixture import or preserve the ordinary absent-file result.
 */
int closepackfile(int handle)
{
    if(handle < 0)
    {
        return -1;
    }
    assert(handle == 1 && fixture_import_open);
    fixture_import_open = 0;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Fixture natives accept integers, so compile-time string mapping is a no-op.
 */
bool Script_MapStringConstants(Instruction *instruction)
{
    (void)instruction;
    return true;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Fail visibly if the parser invokes the production fatal-error path.
 */
void borShutdown(int status, const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    (void)status;
    abort();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Supply the production timer/log symbols when collector hooks are inactive.
 */
uint64_t timer_uticks(void)
{
    return fixture_time;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Keep unrelated parser diagnostics observable without changing measured time.
 */
void writeToLogFile(const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Advance one deterministic microsecond per read to expose profiling overhead.
 */
static uint64_t fixture_clock(void)
{
    ++fixture_clock_reads;
    return fixture_time++;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Retain complete window records for verification after execution finishes.
 */
static void fixture_log_line(const char *line)
{
    size_t length = strlen(line);

    assert(fixture_log_used + length + 2 < sizeof(fixture_log));
    memcpy(fixture_log + fixture_log_used, line, length);
    fixture_log_used += length;
    fixture_log[fixture_log_used++] = '\n';
    fixture_log[fixture_log_used] = '\0';
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Perform deterministic native work while validating compiled argument access.
 */
static HRESULT fixture_work(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    assert(count == 1);
    assert(parameters[0]->vt == VT_INTEGER);
    ++fixture_work_calls;
    fixture_work_sum += parameters[0]->lVal;
    fixture_time += 100;
    ScriptVariant_ChangeType(*result, VT_INTEGER);
    (*result)->lVal = parameters[0]->lVal + 1;
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Reenter real bytecode from a native function, then resume its native work.
 */
static HRESULT fixture_outer(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    (void)parameters;
    assert(count == 0);
    ++fixture_outer_calls;
    fixture_time += 40;
    Interpreter_Reset(&fixture_child);
    assert(SUCCEEDED(Interpreter_EvaluateImmediate(&fixture_child)));
    fixture_time += 60;
    ScriptVariant_ChangeType(*result, VT_INTEGER);
    (*result)->lVal = 1;
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Exercise the real interpreter exception path after timed native execution.
 */
static HRESULT fixture_fail(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    (void)parameters;
    (void)result;
    assert(count == 0);
    ++fixture_error_calls;
    fixture_time += 30;
    return E_FAIL;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Cancel a frame inside native code so enclosing tokens become stale safely.
 */
static HRESULT fixture_cancel(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    (void)parameters;
    (void)result;
    assert(count == 0);
    engine_profile_reset("script_integration_cancel");
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Compile real source and verify names are cached before tokens are destroyed.
 */
static void fixture_compile(Interpreter *interpreter, const char *name,
    const char *source)
{
    char *text = strdup(source);
    int index;

    Interpreter_Init(interpreter, name, &fixture_functions);
    assert(SUCCEEDED(Interpreter_ParseText(interpreter, text, 1, name)));
    assert(SUCCEEDED(Interpreter_CompileInstructions(interpreter)));
    free(text);
    for(index = 0; index < interpreter->theInstructionList.size; ++index)
    {
        Instruction *instruction = interpreter->theInstructionList.solidlist[index];

        assert(!instruction->theToken);
        if(instruction->functionRef)
        {
            assert(instruction->profile_native_id != 0);
            assert(strncmp(engine_profile_native_name(instruction->profile_native_id),
                "profile_", 8) == 0);
        }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Reset collection, preserving precompiled native IDs across configuration.
 */
static void fixture_configure(int enabled)
{
    engine_profile_configure(0, 1, 0);
    fixture_time = 1000;
    fixture_clock_reads = 0;
    fixture_work_calls = 0;
    fixture_work_sum = 0;
    fixture_outer_calls = 0;
    fixture_error_calls = 0;
    fixture_log_used = 0;
    fixture_log[0] = '\0';
    engine_profile_set_hooks(fixture_clock, fixture_log_line);
    engine_profile_configure(enabled, 1, 0);
    engine_profile_script_configure(enabled, 1);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Advance the engine's phase markers to the same draw-script phase used in game.
 */
static uint64_t fixture_begin_frame(void)
{
    uint64_t frame = engine_profile_frame_begin(1, &fixture_scene, &fixture_target);
    ep_phase phase;

    for(phase = EP_PHASE_PRE_WAIT; phase <= EP_PHASE_DRAW_SETUP; ++phase)
    {
        engine_profile_frame_mark(frame, phase);
    }
    return frame;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Finalize the frame interval at the next entry and retain its one-frame report.
 */
static void fixture_finish_frame(uint64_t frame)
{
    engine_profile_frame_mark(frame, EP_PHASE_DRAW_SCRIPT);
    fixture_time += 20;
    engine_profile_frame_end(frame);
    fixture_time += 30;
    engine_profile_frame_begin(1, &fixture_scene, &fixture_target);
    assert(engine_profile_last_report()->frames == 1);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Measure optional host-only smoke benchmarks with a real monotonic clock.
 * This is not a GPU workload or a claim about game performance on user hardware.
 */
static uint64_t fixture_monotonic_ns(void)
{
    struct timespec stamp;

    assert(clock_gettime(CLOCK_MONOTONIC, &stamp) == 0);
    return (uint64_t)stamp.tv_sec * UINT64_C(1000000000) + (uint64_t)stamp.tv_nsec;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Match the production profiler's microsecond clock representation for timing.
 */
static uint64_t fixture_monotonic_us(void)
{
    return fixture_monotonic_ns() / UINT64_C(1000);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Discard benchmark log output so terminal I/O does not dominate the workload.
 */
static void fixture_benchmark_log(const char *line)
{
    (void)line;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Compare identical compiled native-heavy loops with detail off, interval 16
 * and interval 1. Rotate trial order and print observations without flaky
 * timing assertions; ordinary correctness tests never run this benchmark.
 */
static void fixture_benchmark(void)
{
    static const unsigned intervals[] = {0, 16, 1};
    Interpreter interpreter;
    unsigned trial;
    unsigned mode;

    engine_profile_set_hooks(fixture_monotonic_us, fixture_benchmark_log);
    engine_profile_configure(0, EP_MAX_WINDOW, 0);
    List_Init(&fixture_functions);
    List_InsertAfter(&fixture_functions, fixture_work, "profile_work");
    ImportCache_Init(&fixture_functions);
    fixture_compile(&interpreter, "profile_benchmark.c",
        "void main() { int i; int total = 0; "
        "for(i = 0; i < 1024; i++) { total += profile_work(1); } }\n");

    for(trial = 0; trial < 3; ++trial)
    {
        for(mode = 0; mode < 3; ++mode)
        {
            unsigned interval = intervals[(trial + mode) % 3];
            uint64_t start;
            uint64_t end;
            uint64_t frames = 0;

            engine_profile_configure(1, EP_MAX_WINDOW, 0);
            engine_profile_script_configure(interval != 0, interval ? interval : 1);
            fixture_work_calls = 0;
            fixture_work_sum = 0;
            start = fixture_monotonic_ns();
            end = start;
            do
            {
                uint64_t frame = fixture_begin_frame();

                Interpreter_Reset(&interpreter);
                assert(SUCCEEDED(Interpreter_EvaluateImmediate(&interpreter)));
                engine_profile_frame_mark(frame, EP_PHASE_DRAW_SCRIPT);
                engine_profile_frame_end(frame);
                ++frames;
                if((frames & 31) == 0)
                {
                    end = fixture_monotonic_ns();
                }
            }
            while(end - start < UINT64_C(1000000000));

            assert(fixture_work_calls == frames * UINT64_C(1024));
            printf("SCRIPT PROFILE BENCH clock=monotonic_host_us interval=%u "
                "trial=%u frames=%llu native_calls=%u elapsed_ms=%.3f ns_per_native=%.3f\n",
                interval, trial + 1, (unsigned long long)frames, fixture_work_calls,
                (double)(end - start) / 1000000.0,
                (double)(end - start) / fixture_work_calls);
            engine_profile_shutdown();
        }
    }

    Interpreter_Clear(&interpreter);
    ImportCache_Clear();
    List_Clear(&fixture_functions);
    StrCache_Clear();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Cover compiled helper/import calls, aliases, native reentry, errors, frame
 * cancellation and disabled collection without modifying execution semantics.
 */
int main(int argc, char **argv)
{
    Interpreter main_script;
    Interpreter reentrant_script;
    Interpreter error_script;
    Interpreter cancel_script;
    Interpreter entry_script;
    uint64_t frame;
    uint64_t reads;
    unsigned work_id;
    unsigned alias_id;
    unsigned outer_id;
    unsigned fail_id;
    const ep_script_report *report;

    if(argc == 2 && strcmp(argv[1], "--benchmark") == 0)
    {
        fixture_benchmark();
        return 0;
    }
    assert(argc == 1);

    engine_profile_set_hooks(fixture_clock, fixture_log_line);
    engine_profile_configure(0, 1, 0);
    List_Init(&fixture_functions);
    List_InsertAfter(&fixture_functions, fixture_work, "profile_work");
    List_InsertAfter(&fixture_functions, fixture_work, "profile_alias");
    List_InsertAfter(&fixture_functions, fixture_outer, "profile_outer");
    List_InsertAfter(&fixture_functions, fixture_fail, "profile_fail");
    List_InsertAfter(&fixture_functions, fixture_cancel, "profile_cancel");
    ImportCache_Init(&fixture_functions);
    fixture_compile(&main_script, "profile_main.c",
        "#import \"profiling_fixture_import.c\"\n"
        "int helper(int value) { return profile_work(value); }\n"
        "void main() { int i; for(i = 0; i < 3; i++) { helper(i); } "
        "imported_work(); profile_alias(11); }\n");
    fixture_compile(&fixture_child, "profile_child.c",
        "void main() { profile_work(5); }\n");
    fixture_compile(&reentrant_script, "profile_reentrant.c",
        "void main() { profile_outer(); }\n");
    fixture_compile(&error_script, "profile_error.c",
        "void main() { profile_fail(); }\n");
    fixture_compile(&cancel_script, "profile_cancel.c",
        "void main() { profile_cancel(); profile_work(2); }\n");
    fixture_compile(&entry_script, "profile_entries.c",
        "int marker = profile_work(17);\n"
        "void oncreate() { profile_work(3); }\n"
        "void ondestroy() { profile_work(4); }\n"
        "void main() { profile_work(marker); }\n");
    work_id = engine_profile_native_register("profile_work");
    alias_id = engine_profile_native_register("profile_alias");
    outer_id = engine_profile_native_register("profile_outer");
    fail_id = engine_profile_native_register("profile_fail");
    assert(work_id && alias_id && outer_id && fail_id);
    assert(work_id != alias_id);

    fixture_configure(1);
    frame = fixture_begin_frame();
    Interpreter_Reset(&main_script);
    assert(SUCCEEDED(Interpreter_EvaluateImmediate(&main_script)));
    assert(fixture_work_calls == 5 && fixture_work_sum == 21);
    fixture_finish_frame(frame);
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 1 && !report->invalid_frames);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][work_id].calls == 4);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][alias_id].calls == 1);
    assert(report->phases[EP_PHASE_DRAW_SCRIPT].native_calls == 5);
    assert(report->phases[EP_PHASE_DRAW_SCRIPT].interpreter_us > 0);
    assert(strstr(fixture_log, "profile_work"));
    assert(strstr(fixture_log, "profile_alias"));

    /* Entry hooks must also cover global immediate code and direct init/clear
     * evaluation, which bypasses the normal Script_Execute main wrapper. */
    fixture_configure(1);
    frame = fixture_begin_frame();
    Interpreter_Reset(&entry_script);
    assert(SUCCEEDED(Interpreter_EvaluateImmediate(&entry_script)));
    assert(fixture_work_calls == 2 && fixture_work_sum == 35);
    Interpreter_Reset(&entry_script);
    assert(entry_script.pInitEntry);
    entry_script.pCurrentInstruction = entry_script.pInitEntry;
    assert(SUCCEEDED(Interpreter_EvaluateCall(&entry_script)));
    Interpreter_Reset(&entry_script);
    assert(entry_script.pClearEntry);
    entry_script.pCurrentInstruction = entry_script.pClearEntry;
    assert(SUCCEEDED(Interpreter_EvaluateCall(&entry_script)));
    assert(fixture_work_calls == 4 && fixture_work_sum == 42);
    fixture_finish_frame(frame);
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 1 && !report->invalid_frames);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][work_id].calls == 4);

    fixture_configure(1);
    frame = fixture_begin_frame();
    Interpreter_Reset(&reentrant_script);
    assert(SUCCEEDED(Interpreter_EvaluateImmediate(&reentrant_script)));
    assert(fixture_outer_calls == 1 && fixture_work_calls == 1);
    fixture_finish_frame(frame);
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 1 && !report->invalid_frames);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][work_id].calls == 1);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][outer_id].calls == 1);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][outer_id].self_us >= 100);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][outer_id].self_us < 150);
    assert(report->phases[EP_PHASE_DRAW_SCRIPT].native_us < 250);
    assert(strstr(fixture_log, "profile_outer"));
    assert(strstr(fixture_log, "profile_work"));

    fixture_configure(1);
    frame = fixture_begin_frame();
    Interpreter_Reset(&error_script);
    assert(FAILED(Interpreter_EvaluateImmediate(&error_script)));
    assert(fixture_error_calls == 1);
    Interpreter_Reset(&fixture_child);
    assert(SUCCEEDED(Interpreter_EvaluateImmediate(&fixture_child)));
    assert(fixture_work_calls == 1);
    fixture_finish_frame(frame);
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 1 && !report->invalid_frames);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][fail_id].calls == 1);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][work_id].calls == 1);
    assert(strstr(fixture_log, "profile_fail"));

    fixture_configure(1);
    frame = fixture_begin_frame();
    Interpreter_Reset(&cancel_script);
    assert(SUCCEEDED(Interpreter_EvaluateImmediate(&cancel_script)));
    assert(!engine_profile_script_active && fixture_work_calls == 1);
    engine_profile_frame_end(frame);
    frame = fixture_begin_frame();
    Interpreter_Reset(&fixture_child);
    assert(SUCCEEDED(Interpreter_EvaluateImmediate(&fixture_child)));
    fixture_finish_frame(frame);
    report = engine_profile_last_script_report();
    assert(report->sampled_frames == 1 && !report->invalid_frames);
    assert(report->functions[EP_PHASE_DRAW_SCRIPT][work_id].calls == 1);

    fixture_configure(1);
    engine_profile_script_configure(0, 1);
    frame = fixture_begin_frame();
    reads = fixture_clock_reads;
    Interpreter_Reset(&main_script);
    assert(SUCCEEDED(Interpreter_EvaluateImmediate(&main_script)));
    assert(fixture_work_calls == 5 && fixture_work_sum == 21);
    assert(fixture_clock_reads == reads);
    fixture_finish_frame(frame);
    assert(engine_profile_last_script_report()->sampled_frames == 0);

    fixture_configure(0);
    reads = fixture_clock_reads;
    Interpreter_Reset(&main_script);
    assert(SUCCEEDED(Interpreter_EvaluateImmediate(&main_script)));
    assert(fixture_work_calls == 5 && fixture_work_sum == 21);
    assert(fixture_clock_reads == reads);

    Interpreter_Clear(&entry_script);
    Interpreter_Clear(&cancel_script);
    Interpreter_Clear(&error_script);
    Interpreter_Clear(&reentrant_script);
    Interpreter_Clear(&fixture_child);
    Interpreter_Clear(&main_script);
    ImportCache_Clear();
    List_Clear(&fixture_functions);
    StrCache_Clear();
    engine_profile_shutdown();
    puts("Script/native profiler interpreter integration tests passed.");
    return 0;
}
