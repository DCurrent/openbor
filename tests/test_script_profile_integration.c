/*
 * Real parser/interpreter coverage for engine-side script profiling and
 * compile-classified native calls.
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

#ifdef NDEBUG
#error "Interpreter integration tests and benchmarks require assertions enabled."
#endif

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
static Interpreter *fixture_active_interpreter;
static char fixture_native_trace[16384];
static size_t fixture_native_trace_used;
static int fixture_sequence;
static int fixture_shadow_calls;
static Instruction **fixture_failure_ip;
static ScriptVariant fixture_foreign_result;
static int fixture_step_dispatch;
static const char fixture_import_source[] =
    "void imported_work() { profile_work(7); }\n"
    "int profile_shadow(int value) { return value + 200; }\n";

static HRESULT fixture_mapped(ScriptVariant **parameters,
    ScriptVariant **result, int count);
static HRESULT fixture_evaluate_call(Interpreter *interpreter);
static HRESULT fixture_evaluate_immediate(Interpreter *interpreter);

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
 * Exercise compile-time argument mapping before native descriptors are built.
 * Other fixture functions retain their original argument representations.
 */
bool Script_MapStringConstants(Instruction *instruction)
{
    if(instruction->functionRef == fixture_mapped)
    {
        ScriptVariant **parameters =
            (ScriptVariant **)instruction->theRefList->solidlist;

        assert(instruction->theRef->lVal == 1);
        assert(parameters[0]->vt == VT_STR);
        assert(strcmp(StrCache_Get(parameters[0]->strVal), "mapped_token") == 0);
        ScriptVariant_ChangeType(parameters[0], VT_INTEGER);
        parameters[0]->lVal = 93;
    }
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
    assert(SUCCEEDED(fixture_evaluate_immediate(&fixture_child)));
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
 * Append a bounded semantic trace without depending on pointer addresses or
 * string-cache slot numbers, which may differ between independent compiles.
 */
static void fixture_trace_append(const char *format, ...)
{
    va_list arguments;
    int length;
    size_t available = sizeof(fixture_native_trace) - fixture_native_trace_used;

    va_start(arguments, format);
    length = vsnprintf(fixture_native_trace + fixture_native_trace_used,
        available, format, arguments);
    va_end(arguments);
    assert(length >= 0 && (size_t)length < available);
    fixture_native_trace_used += (size_t)length;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Check the observable native-call context and record evaluated arguments.
 * Deliberately change the shared cursor so later invocations must reset it.
 */
static void fixture_native_enter(const char *name, ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    Instruction *instruction;
    int index;

    assert(fixture_active_interpreter);
    assert(fixture_active_interpreter->pCurrentCall);
    assert(fixture_active_interpreter->pCurrentCall ==
        fixture_active_interpreter->pCurrentInstruction);
    instruction = *fixture_active_interpreter->pCurrentCall;
    assert(instruction->theVal == *result);
    assert(*result);
    assert(instruction->theRef->lVal == count);
    assert(instruction->theRefList->size == count);
    assert(instruction->theRefList->index == 0);
    assert(instruction->theRefList->solidlist == (void **)parameters);
    instruction->theRefList->index = 19;

    fixture_trace_append("%s(", name);
    for(index = 0; index < count; ++index)
    {
        ScriptVariant *value = parameters[index];

        assert(value);
        fixture_trace_append("%u:", (unsigned)value->vt);
        switch(value->vt)
        {
        case VT_EMPTY:
            fixture_trace_append("empty");
            break;
        case VT_INTEGER:
            fixture_trace_append("%ld", (long)value->lVal);
            break;
        case VT_INTEGER64:
            fixture_trace_append("%lld", (long long)value->llVal);
            break;
        case VT_UINTEGER64:
            fixture_trace_append("%llu", (unsigned long long)value->ullVal);
            break;
        case VT_DECIMAL:
            fixture_trace_append("%.17g", value->dblVal);
            break;
        case VT_STR:
            fixture_trace_append("%s", StrCache_Get(value->strVal));
            break;
        case VT_PTR:
            assert(value->ptrVal == &fixture_target);
            fixture_trace_append("fixture_target");
            break;
        default:
            assert(!"unexpected fixture variant type");
            break;
        }
        fixture_trace_append(";");
    }
    fixture_trace_append(")\n");
    fixture_time += 7;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Cover zero-argument pointer tables and independent alias profiler IDs.
 */
static HRESULT fixture_zero(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    fixture_native_enter("zero", parameters, result, count);
    assert(count == 0 && !parameters);
    ScriptVariant_ChangeType(*result, VT_INTEGER);
    (*result)->lVal = 7;
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Record values, including empty/string/pointer carriers, and return no value.
 */
static HRESULT fixture_sink(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    fixture_native_enter("sink", parameters, result, count);
    *result = NULL;
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Copy changing carriers through the same compiled return slot. Strings use
 * the production reference-counted copy and cleanup operations.
 */
static HRESULT fixture_identity(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    fixture_native_enter("identity", parameters, result, count);
    assert(count == 1);
    ScriptVariant_Copy(*result, parameters[0]);
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Alternate value, void, and redirected returns from one call site. Local
 * result-pointer changes must not replace the descriptor's reusable storage.
 */
static HRESULT fixture_optional(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    fixture_native_enter("optional", parameters, result, count);
    assert(count == 1 && parameters[0]->vt == VT_INTEGER);
    if(parameters[0]->lVal == 3)
    {
        ScriptVariant_ChangeType(&fixture_foreign_result, VT_INTEGER);
        fixture_foreign_result.lVal = 777;
        *result = &fixture_foreign_result;
    }
    else if(parameters[0]->lVal & 1)
    {
        *result = NULL;
    }
    else if(parameters[0]->lVal == 0)
    {
        ScriptVariant_ChangeType(*result, VT_STR);
        (*result)->strVal = StrCache_CreateNewFrom("retained");
    }
    else
    {
        ScriptVariant_ChangeType(*result, VT_INTEGER);
        (*result)->lVal = 42;
    }
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Supply a known pointer carrier without allocating game objects.
 */
static HRESULT fixture_pointer(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    fixture_native_enter("pointer", parameters, result, count);
    assert(count == 0);
    ScriptVariant_ChangeType(*result, VT_PTR);
    (*result)->ptrVal = &fixture_target;
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Clear a reused return slot so empty variants follow the same native ABI.
 */
static HRESULT fixture_empty(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    fixture_native_enter("empty", parameters, result, count);
    assert(count == 0);
    ScriptVariant_Clear(*result);
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Make argument evaluation order externally visible before outer dispatch.
 */
static HRESULT fixture_next(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    fixture_native_enter("next", parameters, result, count);
    assert(count == 0);
    ScriptVariant_ChangeType(*result, VT_INTEGER);
    (*result)->lVal = ++fixture_sequence;
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Validate an argument table larger than legacy fixed-size native buffers.
 */
static HRESULT fixture_many(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    int index;
    int total = 0;

    fixture_native_enter("many", parameters, result, count);
    assert(count == 80);
    for(index = 0; index < count; ++index)
    {
        assert(parameters[index]->vt == VT_INTEGER);
        assert(parameters[index]->lVal == index);
        total += parameters[index]->lVal;
    }
    ScriptVariant_ChangeType(*result, VT_INTEGER);
    (*result)->lVal = total;
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Assert that string mapping preceded descriptor construction and execution.
 */
static HRESULT fixture_mapped(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    fixture_native_enter("mapped", parameters, result, count);
    assert(count == 1 && parameters[0]->vt == VT_INTEGER);
    assert(parameters[0]->lVal == 93);
    ScriptVariant_Copy(*result, parameters[0]);
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * The script/import override cases must never reach the registered native.
 */
static HRESULT fixture_shadow(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    fixture_native_enter("shadow", parameters, result, count);
    ++fixture_shadow_calls;
    ScriptVariant_ChangeType(*result, VT_INTEGER);
    (*result)->lVal = -1;
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Reenter the same interpreter through a zero-argument lifecycle entry.
 * Preserve the host's existing execution context around that explicit call.
 */
static HRESULT fixture_reenter(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    Interpreter saved;
    HRESULT status;

    fixture_native_enter("reenter", parameters, result, count);
    assert((count == 0 || count == 1) && fixture_active_interpreter->pClearEntry);
    saved = *fixture_active_interpreter;
    fixture_active_interpreter->pCurrentInstruction =
        fixture_active_interpreter->pClearEntry;
    fixture_active_interpreter->bCallCompleted = FALSE;
    status = fixture_evaluate_call(fixture_active_interpreter);
    assert(count ? FAILED(status) : SUCCEEDED(status));
    assert(fixture_active_interpreter->pCurrentCall == saved.pCurrentCall);
    *fixture_active_interpreter = saved;
    if(FAILED(status))
    {
        fixture_failure_ip = saved.pCurrentInstruction +
            (*saved.pCurrentInstruction)->step;
        *result = NULL;
        return E_FAIL;
    }
    ScriptVariant_ChangeType(*result, VT_INTEGER);
    (*result)->lVal = 9;
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Exercise native-driven reset without changing the generic reset contract.
 */
static HRESULT fixture_reset_self(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    fixture_native_enter("reset", parameters, result, count);
    assert(count == 0);
    Interpreter_Reset(fixture_active_interpreter);
    ScriptVariant_ChangeType(*result, VT_INTEGER);
    (*result)->lVal = 8;
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Match system_exit's bReset side effect so subsequent bytecode must not run.
 */
static HRESULT fixture_exit(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    fixture_native_enter("exit", parameters, result, count);
    assert(count == 0);
    fixture_active_interpreter->bReset = FALSE;
    *result = NULL;
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Move the current instruction inside native code. Successful calls restore
 * the caller position; failed calls retain the native's modified position.
 */
static HRESULT fixture_redirect(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    Instruction *instruction;

    fixture_native_enter("redirect", parameters, result, count);
    assert(count >= 1 && parameters[0]->vt == VT_INTEGER);
    assert(fixture_active_interpreter->pClearEntry);
    instruction = *fixture_active_interpreter->pCurrentCall;
    fixture_failure_ip = fixture_active_interpreter->pClearEntry + instruction->step;
    fixture_active_interpreter->pCurrentInstruction =
        fixture_active_interpreter->pClearEntry;
    *result = NULL;
    return parameters[0]->lVal ? E_FAIL : S_OK;
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
            NativeCall *native = instruction->nativeCall;

            assert(instruction->OpCode == CALL_NATIVE);
            assert(native);
            assert(native->function == instruction->functionRef);
            assert(native->parameters ==
                (ScriptVariant **)instruction->theRefList->solidlist);
            assert(native->result == instruction->theVal);
            assert(native->parameter_count == instruction->theRef->lVal);
            assert(native->parameter_index == &instruction->theRefList->index);
            assert(native->profile_native_id == instruction->profile_native_id);
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
 * Select generic or dedicated dispatch without changing compiled operands.
 * Follow script/import control-flow targets so imported native calls use the
 * same mode. Descriptor ownership is retained and restored before destruction.
 */
static unsigned fixture_native_mode(Interpreter *interpreter, int native)
{
    Instruction **pending[4096];
    Instruction *visited[4096];
    size_t pending_count = 0;
    size_t visited_count = 0;
    unsigned changed = 0;
    int index;

    for(index = 0; index < interpreter->theInstructionList.size; ++index)
    {
        assert(pending_count < sizeof(pending) / sizeof(*pending));
        pending[pending_count++] = (Instruction **)
            &interpreter->theInstructionList.solidlist[index];
    }
    while(pending_count)
    {
        Instruction **position = pending[--pending_count];
        Instruction *instruction = *position;
        size_t seen;

        for(seen = 0; seen < visited_count; ++seen)
        {
            if(visited[seen] == instruction)
            {
                break;
            }
        }
        if(seen != visited_count)
        {
            continue;
        }
        assert(visited_count < sizeof(visited) / sizeof(*visited));
        visited[visited_count++] = instruction;
        if(instruction->functionRef)
        {
            assert(instruction->OpCode == CALL || instruction->OpCode == CALL_NATIVE);
            assert(instruction->nativeCall);
            instruction->OpCode = native ? CALL_NATIVE : CALL;
            ++changed;
        }

        /* Imported entries live in another interpreter's pointer table.
         * The compiled target and its own successor links remain valid for
         * the lifetime of ImportCache. RET terminates each reachable body. */
        if(instruction->jumpTargetType ||
            (instruction->OpCode == CALL && !instruction->functionRef))
        {
            if(instruction->ptheJumpTarget)
            {
                assert(pending_count < sizeof(pending) / sizeof(*pending));
                pending[pending_count++] = instruction->ptheJumpTarget;
            }
        }
        if(instruction->OpCode != RET && instruction->OpCode != ERR &&
            instruction->OpCode != DEFERRED)
        {
            assert(instruction->step);
            assert(pending_count < sizeof(pending) / sizeof(*pending));
            pending[pending_count++] = position + instruction->step;
        }
    }
    return changed;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Preserve the previous per-opcode function boundary as a test-side oracle.
 * Nested script calls still enter production evaluation; a separate parent-
 * revision build verifies those paths against the prior complete interpreter.
 */
static HRESULT fixture_evaluate_call(Interpreter *interpreter)
{
    HRESULT status = S_OK;
    ep_script_scope scope = {0, 0, 0};

    if(!fixture_step_dispatch)
    {
        return Interpreter_EvaluateCall(interpreter);
    }
    if(engine_profile_script_active)
    {
        scope = engine_profile_script_begin();
    }
    while(SUCCEEDED(status) && !interpreter->bCallCompleted)
    {
        status = Interpreter_EvalInstruction(interpreter);
    }
    if(scope.frame_id)
    {
        engine_profile_script_end(scope);
    }
    return status;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Reproduce the original immediate scan and main-entry behavior using public
 * single-step dispatch. Do not assume initializers run only once or replace
 * the scan with instruction-step traversal, which has different semantics.
 */
static HRESULT fixture_evaluate_immediate(Interpreter *interpreter)
{
    HRESULT status = S_OK;
    ep_script_scope scope = {0, 0, 0};
    int immediate = 0;
    int index;

    if(!fixture_step_dispatch)
    {
        return Interpreter_EvaluateImmediate(interpreter);
    }
    if(engine_profile_script_active)
    {
        scope = engine_profile_script_begin();
    }
    if(interpreter->bHasImmediateCode)
    {
        for(index = 0; index < interpreter->theInstructionList.size; ++index)
        {
            Instruction *instruction = interpreter->theInstructionList.solidlist[index];

            if(instruction->OpCode == IMMEDIATE)
            {
                immediate = 1;
            }
            else if(instruction->OpCode == DEFERRED)
            {
                immediate = 0;
            }
            if(immediate)
            {
                interpreter->pCurrentInstruction = (Instruction **)
                    &interpreter->theInstructionList.solidlist[index];
                status = Interpreter_EvalInstruction(interpreter);
            }
            if(FAILED(status))
            {
                break;
            }
        }
    }
    if(!interpreter->bMainCompleted)
    {
        if(interpreter->pMainEntry)
        {
            interpreter->pCurrentInstruction = interpreter->pMainEntry;
            status = fixture_evaluate_call(interpreter);
        }
        else
        {
            status = E_FAIL;
        }
        if(SUCCEEDED(status))
        {
            interpreter->bMainCompleted = TRUE;
        }
        interpreter->bCallCompleted = FALSE;
    }
    interpreter->bReset = FALSE;
    if(scope.frame_id)
    {
        engine_profile_script_end(scope);
    }
    return status;
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

typedef struct fixture_native_observation
{
    char trace[16384];
    ep_native_stat functions[EP_SCRIPT_MAX_FUNCTIONS];
    ep_script_phase phase;
    uint64_t clock_reads;
    unsigned main_completed;
    unsigned call_completed;
    unsigned reset;
} fixture_native_observation;

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Compare generic/native calls and normal/single-step execution of identical
 * source. Repeated execution reuses operand/result slots, including strings.
 */
static void fixture_compare_native_source(const char *name, const char *source,
    int fails, const char *expected_trace)
{
    fixture_native_observation observations[4];
    int mode;

    memset(observations, 0, sizeof(observations));
    for(mode = 0; mode < 4; ++mode)
    {
        Interpreter interpreter;
        fixture_native_observation *observation = &observations[mode];
        unsigned repetition;
        int native = mode & 1;

        fixture_step_dispatch = mode >> 1;
        fixture_compile(&interpreter, name, source);
        assert(fixture_native_mode(&interpreter, native) > 0);
        for(repetition = 0; repetition < 2; ++repetition)
        {
            const ep_script_report *report;
            uint64_t frame;
            HRESULT status;
            unsigned id;

            fixture_configure(1);
            fixture_native_trace_used = 0;
            fixture_native_trace[0] = '\0';
            fixture_sequence = 0;
            fixture_shadow_calls = 0;
            fixture_failure_ip = NULL;
            fixture_active_interpreter = &interpreter;
            frame = fixture_begin_frame();
            Interpreter_Reset(&interpreter);
            status = fixture_evaluate_immediate(&interpreter);
            assert(fails ? FAILED(status) : SUCCEEDED(status));
            assert(!interpreter.pCurrentCall);
            assert(!fixture_shadow_calls);
            if(fails && fixture_failure_ip)
            {
                assert(interpreter.pCurrentInstruction == fixture_failure_ip);
            }
            if(strstr(name, "immediate_revisit"))
            {
                /* Initializers repeat even when main has already completed.
                 * A second immediate evaluation deliberately has no reset. */
                assert(SUCCEEDED(fixture_evaluate_immediate(&interpreter)));
            }
            if(strstr(name, "entries"))
            {
                Interpreter_Reset(&interpreter);
                interpreter.pCurrentInstruction = interpreter.pInitEntry;
                assert(SUCCEEDED(fixture_evaluate_call(&interpreter)));
                Interpreter_Reset(&interpreter);
                interpreter.pCurrentInstruction = interpreter.pClearEntry;
                assert(SUCCEEDED(fixture_evaluate_call(&interpreter)));
                assert(!interpreter.pCurrentCall);
            }
            if(!strstr(fixture_native_trace, expected_trace))
            {
                fprintf(stderr, "Native fixture %s mode=%d missing trace %s:\n%s",
                    name, mode, expected_trace, fixture_native_trace);
                assert(!"missing expected native fixture trace");
            }
            if(strstr(name, "exit") || fails)
            {
                assert(!strstr(fixture_native_trace, "999"));
            }
            if(strstr(name, "redirect_success"))
            {
                assert(!strstr(fixture_native_trace, "1000"));
            }
            fixture_finish_frame(frame);
            report = engine_profile_last_script_report();
            assert(report->sampled_frames == 1 && !report->invalid_frames);
            assert(!report->error_flags);
            if(repetition == 0)
            {
                strcpy(observation->trace, fixture_native_trace);
            }
            else
            {
                assert(strcmp(observation->trace, fixture_native_trace) == 0);
            }
            for(id = 0; id < EP_SCRIPT_MAX_FUNCTIONS; ++id)
            {
                observation->functions[id].calls +=
                    report->functions[EP_PHASE_DRAW_SCRIPT][id].calls;
                observation->functions[id].self_us +=
                    report->functions[EP_PHASE_DRAW_SCRIPT][id].self_us;
            }
            observation->phase.interpreter_us +=
                report->phases[EP_PHASE_DRAW_SCRIPT].interpreter_us;
            observation->phase.native_us +=
                report->phases[EP_PHASE_DRAW_SCRIPT].native_us;
            observation->phase.native_calls +=
                report->phases[EP_PHASE_DRAW_SCRIPT].native_calls;
            observation->clock_reads += fixture_clock_reads;
            observation->main_completed = interpreter.bMainCompleted;
            observation->call_completed = interpreter.bCallCompleted;
            observation->reset = interpreter.bReset;
        }

        /* Neither disabled detail timing nor a disabled master profiler may
         * introduce a per-native timer read through the dedicated opcode. */
        for(repetition = 0; repetition < 2; ++repetition)
        {
            uint64_t reads;
            HRESULT status;

            fixture_configure(repetition == 0);
            engine_profile_script_configure(0, 1);
            fixture_native_trace_used = 0;
            fixture_native_trace[0] = '\0';
            fixture_sequence = 0;
            fixture_begin_frame();
            reads = fixture_clock_reads;
            Interpreter_Reset(&interpreter);
            status = fixture_evaluate_immediate(&interpreter);
            assert(fails ? FAILED(status) : SUCCEEDED(status));
            assert(fixture_clock_reads == reads);
            assert(!interpreter.pCurrentCall);
        }

        /* Legacy dispatch still owns its allocated descriptor. Restore its
         * owning opcode before the ordinary production cleanup path runs. */
        fixture_native_mode(&interpreter, 1);
        fixture_active_interpreter = NULL;
        Interpreter_Clear(&interpreter);
    }
    fixture_step_dispatch = 0;
    for(mode = 1; mode < 4; ++mode)
    {
        assert(strcmp(observations[0].trace, observations[mode].trace) == 0);
        assert(memcmp(observations[0].functions, observations[mode].functions,
            sizeof(observations[0].functions)) == 0);
        assert(memcmp(&observations[0].phase, &observations[mode].phase,
            sizeof(observations[0].phase)) == 0);
        assert(observations[0].clock_reads == observations[mode].clock_reads);
        assert(observations[0].main_completed == observations[mode].main_completed);
        assert(observations[0].call_completed == observations[mode].call_completed);
        assert(observations[0].reset == observations[mode].reset);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Exercise language-level contracts affected by bypassing generic native CALL
 * setup, with real source parsing, binding, execution, and destruction.
 */
static void fixture_native_regressions(void)
{
    char many_source[2048];
    size_t used;
    int index;

    List_InsertAfter(&fixture_functions, fixture_zero, "profile_zero");
    List_InsertAfter(&fixture_functions, fixture_zero, "profile_zero_alias");
    List_InsertAfter(&fixture_functions, fixture_sink, "profile_sink");
    List_InsertAfter(&fixture_functions, fixture_identity, "profile_identity");
    List_InsertAfter(&fixture_functions, fixture_optional, "profile_optional");
    List_InsertAfter(&fixture_functions, fixture_pointer, "profile_pointer");
    List_InsertAfter(&fixture_functions, fixture_empty, "profile_empty");
    List_InsertAfter(&fixture_functions, fixture_next, "profile_next");
    List_InsertAfter(&fixture_functions, fixture_many, "profile_many");
    List_InsertAfter(&fixture_functions, fixture_mapped, "profile_mapped");
    List_InsertAfter(&fixture_functions, fixture_shadow, "profile_shadow");
    List_InsertAfter(&fixture_functions, fixture_reenter, "profile_reenter");
    List_InsertAfter(&fixture_functions, fixture_reset_self, "profile_reset_self");
    List_InsertAfter(&fixture_functions, fixture_exit, "profile_exit");
    List_InsertAfter(&fixture_functions, fixture_redirect, "profile_redirect");

    fixture_compare_native_source("native_values.c",
        "void main() { void value; int i; "
        "profile_sink(profile_zero(), profile_zero_alias()); "
        "profile_sink(profile_mapped(\"mapped_token\")); "
        "for(i = 0; i < 6; i++) { "
        "if(i == 0) { value = \"first\"; } "
        "else if(i == 1) { value = 42; } "
        "else if(i == 2) { value = 1.25; } "
        "else if(i == 3) { value = 18446744073709551615u; } "
        "else if(i == 4) { value = profile_pointer(); } "
        "else { value = profile_empty(); } "
        "profile_sink(profile_identity(value)); } "
        "for(i = 0; i < 5; i++) { profile_sink(profile_optional(i)); } "
        "profile_sink(profile_next(), profile_next(), profile_next()); "
        "i = 5; profile_sink(i, i++, i); }\n",
        0, "sink(1:3;1:2;1:1;)");

    used = (size_t)snprintf(many_source, sizeof(many_source),
        "void main() { profile_sink(profile_many(");
    for(index = 0; index < 80; ++index)
    {
        int written = snprintf(many_source + used, sizeof(many_source) - used,
            "%s%d", index ? "," : "", index);

        assert(written > 0 && (size_t)written < sizeof(many_source) - used);
        used += (size_t)written;
    }
    assert(snprintf(many_source + used, sizeof(many_source) - used,
        ")); }\n") > 0);
    fixture_compare_native_source("native_many.c", many_source, 0, "sink(1:3160;)");

    fixture_compare_native_source("native_local_override.c",
        "#import \"profiling_fixture_import.c\"\n"
        "int profile_shadow(int value) { return value + 100; }\n"
        "void main() { profile_sink(profile_shadow(3)); imported_work(); }\n",
        0, "sink(1:103;)");
    fixture_compare_native_source("native_import_override.c",
        "#import \"profiling_fixture_import.c\"\n"
        "void main() { profile_sink(profile_shadow(3)); imported_work(); }\n",
        0, "sink(1:203;)");
    fixture_compare_native_source("native_reentry.c",
        "void ondestroy() { profile_sink(71); }\n"
        "void main() { profile_sink(profile_reenter()); profile_sink(72); }\n",
        0, "sink(1:71;)");
    fixture_compare_native_source("native_reentry_failure.c",
        "void ondestroy() { profile_redirect(1); profile_sink(999); }\n"
        "void main() { profile_reenter(1); profile_sink(999); }\n",
        1, "redirect(1:1;)");
    fixture_compare_native_source("native_entries.c",
        "int marker = profile_zero();\n"
        "void oncreate() { profile_sink(501); }\n"
        "void ondestroy() { profile_sink(502); }\n"
        "void main() { profile_sink(marker); }\n",
        0, "sink(1:502;)");
    fixture_compare_native_source("native_reset.c",
        "void main() { profile_sink(profile_reset_self()); profile_sink(80); }\n",
        0, "sink(1:80;)");
    fixture_compare_native_source("native_exit.c",
        "void main() { profile_exit(); profile_sink(999); }\n",
        0, "exit()");
    fixture_compare_native_source("native_redirect_success.c",
        "void ondestroy() { profile_sink(1000); }\n"
        "void main() { profile_redirect(0); profile_sink(99); }\n",
        0, "sink(1:99;)");
    fixture_compare_native_source("native_redirect_failure.c",
        "void ondestroy() { profile_sink(1000); }\n"
        "void main() { profile_redirect(1); profile_sink(999); }\n",
        1, "redirect(1:1;)");
    fixture_compare_native_source("native_error_arguments.c",
        "void ondestroy() { profile_sink(1000); }\n"
        "void main() { profile_redirect(1, \"argument\", 1.25); profile_sink(999); }\n",
        1, "redirect(1:1;32:argument;2:1.25;)");
    fixture_compare_native_source("dispatch_control_flow.c",
        "int inner(int value) { if(value < 0) { return -value; } return value + 1; }\n"
        "int outer(int value) { return inner(value) + 2; }\n"
        "void main() { int i; int j; int total = 0; int bits; "
        "for(i = 0; i < 5; i++) { if(i == 1) { continue; } "
        "for(j = 0; j < 3; j++) { if(j == 2) { break; } total += outer(i + j); } } "
        "while(total < 48) { total++; } "
        "do { total++; } while(total < 50); "
        "switch(total) { case 49: total = 90; break; "
        "case 50: total += 4; default: total += 1; } "
        "profile_sink(total, inner(-4)); "
        "bits = (3 << 2) | 1; if(bits == 13 && total >= 55) { profile_sink(601); } }\n",
        0, "sink(1:55;1:4;)");
    fixture_compare_native_source("dispatch_immediate_exit.c",
        "int marker = profile_exit();\n"
        "int second = profile_zero();\n"
        "void main() { profile_sink(999); }\n",
        0, "exit()\nzero()");
    fixture_compare_native_source("dispatch_immediate_failure.c",
        "int marker = profile_fail();\n"
        "void main() { profile_sink(81); }\n",
        0, "sink(1:81;)");
    fixture_compare_native_source("dispatch_immediate_revisit.c",
        "int marker = profile_next();\n"
        "void main() { profile_sink(marker); }\n",
        0, "next()\nsink(1:1;)\nnext()");
    assert(engine_profile_native_register("profile_zero") !=
        engine_profile_native_register("profile_zero_alias"));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Verify instruction-boundary behavior directly, independent of parser output.
 * Hand-built targets intentionally use different source/destination steps so
 * a loop rewrite cannot silently advance from the wrong instruction record.
 */
static void fixture_dispatch_steps(void)
{
    int mode;

    fixture_configure(0);
    for(mode = 0; mode < 2; ++mode)
    {
        Interpreter interpreter;
        Instruction instructions[10];
        Instruction *tape[10];
        Instruction *empty_instruction = NULL;
        ScriptVariant value;
        ScriptVariant target;
        ScriptVariant condition;
        ScriptVariant result;
        unsigned index;

        memset(&interpreter, 0, sizeof(interpreter));
        memset(instructions, 0, sizeof(instructions));
        ScriptVariant_Init(&value);
        ScriptVariant_Init(&target);
        ScriptVariant_Init(&condition);
        ScriptVariant_Init(&result);
        ScriptVariant_ChangeType(&value, VT_INTEGER);
        value.lVal = 42;
        for(index = 0; index < sizeof(tape) / sizeof(*tape); ++index)
        {
            tape[index] = &instructions[index];
            instructions[index].OpCode = NOOP;
            instructions[index].step = 1;
        }
        fixture_step_dispatch = mode;

        /* Public single-step execution must still run one opcode when the
         * run-to-completion flag is already set. It must not execute a second. */
        instructions[0].OpCode = SAVE;
        instructions[0].theRef = &target;
        instructions[0].theRef2 = &value;
        instructions[0].step = 2;
        instructions[2].OpCode = ERR;
        interpreter.pCurrentInstruction = tape;
        interpreter.bCallCompleted = TRUE;
        assert(SUCCEEDED(Interpreter_EvalInstruction(&interpreter)));
        assert(target.vt == VT_INTEGER && target.lVal == 42);
        assert(interpreter.pCurrentInstruction == tape + 2);
        assert(interpreter.bCallCompleted);

        /* In contrast, an already-completed run does not fetch any opcode.
         * NULL is intentional here: evaluating even one instruction would fail. */
        interpreter.pCurrentInstruction = NULL;
        assert(SUCCEEDED(fixture_evaluate_call(&interpreter)));
        assert(!interpreter.pCurrentInstruction);

        /* A null instruction slot is a no-op in the single-step contract.
         * Do not run a completion loop on it, because it cannot advance. */
        interpreter.pCurrentInstruction = &empty_instruction;
        interpreter.bCallCompleted = FALSE;
        assert(SUCCEEDED(Interpreter_EvalInstruction(&interpreter)));
        assert(interpreter.pCurrentInstruction == &empty_instruction);

        /* Skipped instructions include deliberate errors. RET must advance
         * with its own step after setting the completion flag. */
        instructions[0].step = 3;
        instructions[1].OpCode = ERR;
        instructions[3].OpCode = RET;
        instructions[3].step = 2;
        interpreter.pCurrentInstruction = tape;
        assert(SUCCEEDED(fixture_evaluate_call(&interpreter)));
        assert(interpreter.bCallCompleted);
        assert(interpreter.pCurrentInstruction == tape + 5);

        /* Both sides of a conditional branch must apply the executed
         * branch's step, not the target instruction's differently sized step. */
        instructions[0].OpCode = Branch_TRUE;
        instructions[0].theRef = &condition;
        instructions[0].step = 2;
        instructions[0].ptheJumpTarget = tape + 3;
        instructions[2].OpCode = SAVE;
        instructions[2].theRef = &target;
        instructions[2].theRef2 = &value;
        instructions[3].step = 1;
        instructions[5].OpCode = RET;
        for(index = 0; index < 2; ++index)
        {
            ScriptVariant_ChangeType(&target, VT_INTEGER);
            target.lVal = 0;
            ScriptVariant_ChangeType(&condition, VT_INTEGER);
            condition.lVal = index;
            interpreter.pCurrentInstruction = tape;
            interpreter.bCallCompleted = FALSE;
            assert(SUCCEEDED(fixture_evaluate_call(&interpreter)));
            assert(target.lVal == (index ? 0 : 42));
            assert(interpreter.pCurrentInstruction == tape + (index ? 6 : 4));
        }

        instructions[0].OpCode = JUMP;
        interpreter.pCurrentInstruction = tape;
        interpreter.bCallCompleted = FALSE;
        assert(SUCCEEDED(fixture_evaluate_call(&interpreter)));
        assert(interpreter.pCurrentInstruction == tape + 6);

        /* JUMPR retains the source instruction as the return-value location.
         * RET copies it to the enclosing call result before advancing. */
        instructions[0].OpCode = JUMPR;
        instructions[0].theRef = &value;
        instructions[9].theVal = &result;
        instructions[5].step = 2;
        interpreter.pCurrentCall = tape + 9;
        interpreter.pCurrentInstruction = tape;
        interpreter.bCallCompleted = FALSE;
        assert(SUCCEEDED(fixture_evaluate_call(&interpreter)));
        assert(result.vt == VT_INTEGER && result.lVal == 42);
        assert(!interpreter.pReturnEntry);
        assert(interpreter.pCurrentCall == tape + 9);
        assert(interpreter.pCurrentInstruction == tape + 7);

        /* Failed and unknown instructions still advance exactly once before
         * the running loop observes E_FAIL and exits. */
        instructions[0].OpCode = ERR;
        instructions[0].step = 3;
        interpreter.pCurrentInstruction = tape;
        interpreter.bCallCompleted = FALSE;
        assert(FAILED(fixture_evaluate_call(&interpreter)));
        assert(interpreter.pCurrentInstruction == tape + 3);
        assert(!interpreter.bCallCompleted);
        instructions[0].OpCode = OPCODE_END + 1;
        interpreter.pCurrentInstruction = tape;
        assert(FAILED(fixture_evaluate_call(&interpreter)));
        assert(interpreter.pCurrentInstruction == tape + 3);

        ScriptVariant_Clear(&result);
        ScriptVariant_Clear(&condition);
        ScriptVariant_Clear(&target);
        ScriptVariant_Clear(&value);
    }
    fixture_step_dispatch = 0;
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
 * Compare identical compiled workloads through generic and dedicated native
 * dispatch with all profiling disabled. Report observations only; elapsed time
 * is intentionally not a correctness assertion or a prediction of game FPS.
 */
static void fixture_native_benchmark(void)
{
    Interpreter interpreter;
    unsigned trial;
    unsigned order;

    engine_profile_set_hooks(fixture_monotonic_us, fixture_benchmark_log);
    engine_profile_configure(0, EP_MAX_WINDOW, 0);
    engine_profile_script_configure(0, 1);
    List_Init(&fixture_functions);
    List_InsertAfter(&fixture_functions, fixture_work, "profile_work");
    ImportCache_Init(&fixture_functions);
    fixture_compile(&interpreter, "native_benchmark.c",
        "void main() { int i; int total = 0; "
        "for(i = 0; i < 1024; i++) { total += profile_work(1); } "
        "profile_work(total); }\n");

    for(trial = 0; trial < 5; ++trial)
    {
        for(order = 0; order < 2; ++order)
        {
            unsigned native = (trial + order) & 1;
            uint64_t evaluations = 0;
            uint64_t start;
            uint64_t end;

            assert(fixture_native_mode(&interpreter, (int)native) == 2);
            fixture_work_calls = 0;
            fixture_work_sum = 0;
            start = fixture_monotonic_ns();
            end = start;
            do
            {
                Interpreter_Reset(&interpreter);
                assert(SUCCEEDED(Interpreter_EvaluateImmediate(&interpreter)));
                ++evaluations;
                if((evaluations & 31) == 0)
                {
                    end = fixture_monotonic_ns();
                }
            }
            while(end - start < UINT64_C(500000000));

            /* Each evaluation makes 1024 calls with argument 1, then one
             * checksum call with the computed sum (1024 * 2 == 2048). */
            assert(fixture_work_calls == evaluations * UINT64_C(1025));
            assert((uint64_t)fixture_work_sum == evaluations * UINT64_C(3072));
            printf("NATIVE CALL BENCH clock=monotonic_host mode=%s trial=%u "
                "evaluations=%llu native_calls=%u elapsed_ms=%.3f "
                "ns_per_native=%.3f\n", native ? "native" : "generic", trial + 1,
                (unsigned long long)evaluations, fixture_work_calls,
                (double)(end - start) / 1000000.0,
                (double)(end - start) / fixture_work_calls);
        }
    }

    fixture_native_mode(&interpreter, 1);
    Interpreter_Clear(&interpreter);
    ImportCache_Clear();
    List_Clear(&fixture_functions);
    StrCache_Clear();
    engine_profile_shutdown();
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
    if(argc == 2 && strcmp(argv[1], "--native-benchmark") == 0)
    {
        fixture_native_benchmark();
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

    fixture_native_regressions();
    fixture_dispatch_steps();

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
    puts("Script/native profiler, native-call and opcode-dispatch integration tests passed.");
    return 0;
}
