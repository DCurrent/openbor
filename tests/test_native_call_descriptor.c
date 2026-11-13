/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Exercise native-call descriptor validation, publication and ownership using
 * production instruction/list/variant code. Link with --wrap=malloc and
 * --wrap=free so optional allocation failure and exact ownership are visible
 * without adding test hooks to the interpreter.
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Instruction.h"

#ifdef NDEBUG
#error "Native-call descriptor tests require assertions enabled."
#endif

#define FIXTURE_ARGUMENT_COUNT 80

/* Keep the pre-descriptor layout explicit so additional instruction-wide
 * storage cannot silently enter this memory-for-speed optimization. */
#pragma pack(4)
typedef struct LegacyInstruction
{
    unsigned OpCode;
    unsigned jumpTargetType;
    unsigned step;
    unsigned profile_native_id;
    Token *theToken;
    CHAR *Label;
    ScriptVariant *theVal;
    ScriptVariant *theRef;
    ScriptVariant *theRef2;
    List *theRefList;
    HRESULT (*functionRef)(ScriptVariant **, ScriptVariant **, int);
    union
    {
        int theJumpTargetIndex;
        Instruction **ptheJumpTarget;
    };
} LegacyInstruction;
#pragma pack()

typedef char fixture_instruction_size_unchanged[
    sizeof(Instruction) == sizeof(LegacyInstruction) ? 1 : -1];

typedef struct DescriptorFixture
{
    Instruction instruction;
    ScriptVariant count;
    ScriptVariant arguments[FIXTURE_ARGUMENT_COUNT];
    int argument_count;
} DescriptorFixture;

static int fixture_watch_allocation;
static int fixture_fail_allocation;
static unsigned fixture_allocation_calls;
static unsigned fixture_safe_allocation_calls;
static uintptr_t fixture_owned_addresses[4];
static unsigned fixture_owned_frees[4];
static uintptr_t fixture_borrowed_addresses[FIXTURE_ARGUMENT_COUNT + 1];
static unsigned fixture_borrowed_count;

void *__real_malloc(size_t size);
void __real_free(void *pointer);

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Intercept only the explicitly armed descriptor allocation. Ordinary
 * fixture construction and the production allocator remain unrestricted.
 */
void *__wrap_malloc(size_t size)
{
    if(fixture_watch_allocation)
    {
        ++fixture_allocation_calls;
        assert(size == sizeof(NativeCall));
        if(fixture_fail_allocation)
        {
            return NULL;
        }
    }
    return __real_malloc(size);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Count each instruction-owned allocation exactly once and reject any free
 * of the borrowed argument/count variants. Integer addresses remain safe to
 * compare after the associated allocation has been released.
 */
void __wrap_free(void *pointer)
{
    uintptr_t address = (uintptr_t)pointer;
    unsigned index;

    if(pointer)
    {
        for(index = 0; index < fixture_borrowed_count; ++index)
        {
            assert(address != fixture_borrowed_addresses[index]);
        }
        for(index = 0; index < 4; ++index)
        {
            if(address == fixture_owned_addresses[index])
            {
                ++fixture_owned_frees[index];
                assert(fixture_owned_frees[index] == 1);
            }
        }
    }
    __real_free(pointer);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Retain production safe-allocation behavior for required storage. Optional
 * descriptor allocation must bypass this fatal-on-failure wrapper entirely.
 */
void *checkAlloc(void *pointer, size_t size, const char *function,
    const char *file, int line)
{
    (void)size;
    (void)function;
    (void)file;
    (void)line;
    if(fixture_watch_allocation)
    {
        ++fixture_safe_allocation_calls;
    }
    assert(pointer);
    return pointer;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Validate the complete cached argument table and return its integer sum.
 * This fixture also verifies a legitimate NULL table for zero arguments.
 */
static HRESULT fixture_native(ScriptVariant **parameters,
    ScriptVariant **result, int count)
{
    int index;
    int sum = 0;

    assert(count >= 0 && count <= FIXTURE_ARGUMENT_COUNT);
    assert(count || !parameters);
    for(index = 0; index < count; ++index)
    {
        assert(parameters[index]);
        assert(parameters[index]->vt == VT_INTEGER);
        sum += parameters[index]->lVal;
    }
    ScriptVariant_ChangeType(*result, VT_INTEGER);
    (*result)->lVal = sum;
    return S_OK;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Build the same owned result/list and borrowed count/argument relationships
 * as a compiled CALL, using the production List solidification path.
 */
static void fixture_init(DescriptorFixture *fixture, int count, unsigned id)
{
    int index;

    assert(count >= 0 && count <= FIXTURE_ARGUMENT_COUNT);
    memset(fixture, 0, sizeof(*fixture));
    fixture->argument_count = count;
    fixture->instruction.OpCode = CALL;
    fixture->instruction.step = 3;
    fixture->instruction.profile_native_id = id;
    fixture->instruction.functionRef = fixture_native;
    fixture->instruction.theRef = &fixture->count;
    ScriptVariant_Init(&fixture->count);
    fixture->count.vt = VT_INTEGER;
    fixture->count.lVal = count;

    fixture->instruction.theVal = malloc(sizeof(*fixture->instruction.theVal));
    ScriptVariant_Init(fixture->instruction.theVal);
    fixture->instruction.theRefList = malloc(sizeof(*fixture->instruction.theRefList));
    List_Init(fixture->instruction.theRefList);
    for(index = 0; index < count; ++index)
    {
        ScriptVariant_Init(&fixture->arguments[index]);
        fixture->arguments[index].vt = VT_INTEGER;
        fixture->arguments[index].lVal = index;
        List_InsertAfter(fixture->instruction.theRefList,
            &fixture->arguments[index], NULL);
    }
    assert(List_Solidify(fixture->instruction.theRefList));
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Observe exactly one helper invocation. No safe allocator calls should be
 * needed when the optional native descriptor is the only new allocation.
 */
static bool fixture_compile(Instruction *instruction, int fail,
    unsigned expected_allocations)
{
    bool result;

    fixture_allocation_calls = 0;
    fixture_safe_allocation_calls = 0;
    fixture_fail_allocation = fail;
    fixture_watch_allocation = 1;
    result = Instruction_CompileNativeCall(instruction);
    fixture_watch_allocation = 0;
    fixture_fail_allocation = 0;
    assert(fixture_allocation_calls == expected_allocations);
    assert(fixture_safe_allocation_calls == 0);
    return result;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Reject invalid/ineligible metadata without allocating or publishing any
 * changes, including to fields that the generic fallback still requires.
 */
static void fixture_expect_rejected(Instruction *instruction)
{
    unsigned char before[sizeof(*instruction)];

    memcpy(before, instruction, sizeof(before));
    assert(!fixture_compile(instruction, 0, 0));
    assert(memcmp(before, instruction, sizeof(before)) == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Verify one-time ownership cleanup and repeated-clear safety while keeping
 * the fixture's borrowed arguments/count available to their original owner.
 */
static void fixture_clear(DescriptorFixture *fixture)
{
    Instruction empty;
    unsigned index;

    memset(fixture_owned_addresses, 0, sizeof(fixture_owned_addresses));
    memset(fixture_owned_frees, 0, sizeof(fixture_owned_frees));
    fixture_owned_addresses[0] = fixture->instruction.OpCode == CALL_NATIVE
        ? (uintptr_t)fixture->instruction.nativeCall : 0;
    fixture_owned_addresses[1] = (uintptr_t)fixture->instruction.theVal;
    fixture_owned_addresses[2] = (uintptr_t)fixture->instruction.theRefList;
    fixture_owned_addresses[3] = fixture->instruction.theRefList
        ? (uintptr_t)fixture->instruction.theRefList->solidlist : 0;
    fixture_borrowed_count = (unsigned)fixture->argument_count + 1;
    fixture_borrowed_addresses[0] = (uintptr_t)&fixture->count;
    for(index = 0; index < (unsigned)fixture->argument_count; ++index)
    {
        fixture_borrowed_addresses[index + 1] =
            (uintptr_t)&fixture->arguments[index];
    }

    Instruction_Clear(&fixture->instruction);
    memset(&empty, 0, sizeof(empty));
    assert(memcmp(&fixture->instruction, &empty, sizeof(empty)) == 0);
    for(index = 0; index < 4; ++index)
    {
        assert(fixture_owned_frees[index] ==
            (fixture_owned_addresses[index] ? 1U : 0U));
    }
    assert(fixture->count.vt == VT_INTEGER);
    assert(fixture->count.lVal == fixture->argument_count);

    Instruction_Clear(&fixture->instruction);
    for(index = 0; index < 4; ++index)
    {
        assert(fixture_owned_frees[index] ==
            (fixture_owned_addresses[index] ? 1U : 0U));
    }
    memset(fixture_owned_addresses, 0, sizeof(fixture_owned_addresses));
    fixture_borrowed_count = 0;
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Exercise zero and large argument counts, profile ID zero, direct payload
 * pointers, fresh live values, diagnostic spelling and idempotent lowering.
 */
static void test_descriptor_payload(void)
{
    const int counts[] = {0, FIXTURE_ARGUMENT_COUNT};
    DescriptorFixture fixture;
    size_t test;

    assert(sizeof(Instruction) == sizeof(LegacyInstruction));
    assert(offsetof(Instruction, nativeCall) ==
        offsetof(LegacyInstruction, theRef2));
    assert(offsetof(Instruction, theRefList) ==
        offsetof(LegacyInstruction, theRefList));
    assert(sizeof(NativeCall) == sizeof(void *) * 4 + sizeof(int) + sizeof(unsigned));
    assert(CALL == 8 && FUNCDECL == 48 && CALL_NATIVE == 49);

    for(test = 0; test < sizeof(counts) / sizeof(*counts); ++test)
    {
        NativeCall *native;
        ScriptVariant *result;
        char text[128];
        int count = counts[test];
        unsigned id = test ? 91U : 0U;

        fixture_init(&fixture, count, id);
        assert(fixture_compile(&fixture.instruction, 0, 1));
        native = fixture.instruction.nativeCall;
        assert(fixture.instruction.OpCode == CALL_NATIVE && native);
        assert(native->function == fixture_native);
        assert(native->parameters ==
            (ScriptVariant **)fixture.instruction.theRefList->solidlist);
        assert(native->result == fixture.instruction.theVal);
        assert(native->parameter_index == &fixture.instruction.theRefList->index);
        assert(native->parameter_count == count);
        assert(native->profile_native_id == id);
        assert(fixture.instruction.step == 3);

        fixture.instruction.theRefList->index = 23;
        *native->parameter_index = 0;
        assert(fixture.instruction.theRefList->index == 0);
        result = native->result;
        assert(SUCCEEDED(native->function(native->parameters, &result,
            native->parameter_count)));
        assert(result->vt == VT_INTEGER);
        assert(result->lVal == count * (count - 1) / 2);
        if(count)
        {
            fixture.arguments[0].lVal = 1000;
            assert(SUCCEEDED(native->function(native->parameters, &result,
                native->parameter_count)));
            assert(result->lVal == count * (count - 1) / 2 + 1000);
        }

        Instruction_ToString(&fixture.instruction, text);
        assert(strcmp(text, "CALL_NATIVE ") == 0);
        fixture_expect_rejected(&fixture.instruction);
        fixture_clear(&fixture);
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Cover every descriptor eligibility guard while restoring original metadata
 * before ordinary cleanup. Rejection must leave generic call ownership intact.
 */
static void test_descriptor_rejections(void)
{
    DescriptorFixture fixture;
    ScriptVariant *result;
    ScriptVariant **parameters;
    List *list;
    Instruction *target;

    assert(!fixture_compile(NULL, 0, 0));
    fixture_init(&fixture, 2, 0);
    result = fixture.instruction.theVal;
    list = fixture.instruction.theRefList;
    parameters = (ScriptVariant **)list->solidlist;
    target = &fixture.instruction;

    fixture.instruction.OpCode = LOAD;
    fixture_expect_rejected(&fixture.instruction);
    fixture.instruction.OpCode = CALL;
    fixture.instruction.functionRef = NULL;
    fixture_expect_rejected(&fixture.instruction);
    fixture.instruction.functionRef = fixture_native;
    fixture.instruction.jumpTargetType = 1;
    fixture_expect_rejected(&fixture.instruction);
    fixture.instruction.jumpTargetType = 0;
    fixture.instruction.ptheJumpTarget = &target;
    fixture_expect_rejected(&fixture.instruction);
    fixture.instruction.ptheJumpTarget = NULL;
    fixture.instruction.theRef2 = &fixture.arguments[0];
    fixture_expect_rejected(&fixture.instruction);
    fixture.instruction.theRef2 = NULL;
    fixture.instruction.theVal = NULL;
    fixture_expect_rejected(&fixture.instruction);
    fixture.instruction.theVal = result;
    fixture.instruction.theRef = NULL;
    fixture_expect_rejected(&fixture.instruction);
    fixture.instruction.theRef = &fixture.count;
    fixture.count.vt = VT_DECIMAL;
    fixture_expect_rejected(&fixture.instruction);
    fixture.count.vt = VT_INTEGER;
    fixture.count.lVal = -1;
    fixture_expect_rejected(&fixture.instruction);
    fixture.count.lVal = 3;
    fixture_expect_rejected(&fixture.instruction);
    fixture.count.lVal = 2;
    fixture.instruction.theRefList = NULL;
    fixture_expect_rejected(&fixture.instruction);
    fixture.instruction.theRefList = list;
    list->size = 1;
    fixture_expect_rejected(&fixture.instruction);
    list->size = 2;
    list->solidlist = NULL;
    fixture_expect_rejected(&fixture.instruction);
    list->solidlist = (void **)parameters;
    parameters[1] = NULL;
    fixture_expect_rejected(&fixture.instruction);
    parameters[1] = &fixture.arguments[1];

    fixture_clear(&fixture);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Fail precisely the optional descriptor allocation. The original CALL must
 * remain byte-for-byte unchanged, then lower normally once memory is available.
 */
static void test_descriptor_allocation_failure(void)
{
    DescriptorFixture fixture;
    unsigned char before[sizeof(Instruction)];
    ScriptVariant *result;

    fixture_init(&fixture, 2, 0);
    memcpy(before, &fixture.instruction, sizeof(before));
    assert(!fixture_compile(&fixture.instruction, 1, 1));
    assert(memcmp(before, &fixture.instruction, sizeof(before)) == 0);
    assert(fixture.instruction.OpCode == CALL);
    result = fixture.instruction.theVal;
    assert(SUCCEEDED(fixture.instruction.functionRef(
        (ScriptVariant **)fixture.instruction.theRefList->solidlist,
        &result, fixture.instruction.theRef->lVal)));
    assert(result->vt == VT_INTEGER && result->lVal == 1);
    assert(fixture_compile(&fixture.instruction, 0, 1));
    fixture_clear(&fixture);
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Instruction cleanup must release its result's string reference without
 * clearing/freeing a borrowed argument that shares the same string storage.
 */
static void test_descriptor_borrowed_string(void)
{
    DescriptorFixture fixture;
    int index;

    fixture_init(&fixture, 2, 0);
    ScriptVariant_ChangeType(&fixture.arguments[0], VT_STR);
    fixture.arguments[0].strVal = StrCache_CreateNewFrom("borrowed argument");
    ScriptVariant_Copy(fixture.instruction.theVal, &fixture.arguments[0]);
    assert(fixture_compile(&fixture.instruction, 0, 1));
    fixture_clear(&fixture);
    assert(fixture.arguments[0].vt == VT_STR);
    assert(strcmp(StrCache_Get(fixture.arguments[0].strVal),
        "borrowed argument") == 0);
    for(index = 0; index < fixture.argument_count; ++index)
    {
        ScriptVariant_Clear(&fixture.arguments[index]);
    }
    StrCache_Clear();
}

/*
 * Damon V. Caskey
 * 2026-11-12
 *
 * Run descriptor contracts independently of parser/runtime integration so an
 * allocator failure cannot be confused with unrelated script initialization.
 */
int main(void)
{
    test_descriptor_payload();
    test_descriptor_rejections();
    test_descriptor_allocation_failure();
    test_descriptor_borrowed_string();
    puts("Native-call descriptor regression tests passed.");
    return 0;
}
