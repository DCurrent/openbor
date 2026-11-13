#!/usr/bin/env bash
# Damon V. Caskey
# 2026-11-12
# Verify the execution boundary in GNU-compatible object files, rather than
# assuming that an inline keyword eliminated a call. The normal test runner
# supplies both debug and release objects built with function sections.
set -euo pipefail

if [[ $# == 0 ]]; then
    printf 'Usage: %s Interpreter-object [...]\n' "$0" >&2
    exit 2
fi

dispatch_objdump=${OBJDUMP:-objdump}
dispatch_nm=${NM:-nm}

for dispatch_object in "$@"; do
    dispatch_symbols=$("$dispatch_nm" "$dispatch_object")

    # Retain the public one-step API, but never emit a second out-of-line
    # implementation of the private body used by the execution loops.
    if ! grep -Eq '[[:space:]]T[[:space:]]+Interpreter_EvalInstruction$' \
        <<< "$dispatch_symbols"; then
        printf 'FAIL: public single-step entry missing in %s\n' "$dispatch_object" >&2
        exit 1
    fi
    if grep -q 'Interpreter_ExecuteInstruction' <<< "$dispatch_symbols"; then
        printf 'FAIL: shared opcode body was emitted out of line in %s\n' \
            "$dispatch_object" >&2
        exit 1
    fi

    for dispatch_loop in Interpreter_EvaluateCall Interpreter_EvaluateImmediate; do
        dispatch_section=".text.$dispatch_loop"
        dispatch_relocations=$(LC_ALL=C "$dispatch_objdump" -r -j "$dispatch_section" \
            "$dispatch_object")

        # A missing section must not look like success. Restrict inspection
        # to these loops: unwind/debug references to the public API are valid.
        if ! grep -Fq "RELOCATION RECORDS FOR [$dispatch_section]" \
            <<< "$dispatch_relocations"; then
            printf 'FAIL: missing relocation section %s in %s\n' \
                "$dispatch_section" "$dispatch_object" >&2
            exit 1
        fi
        if grep -Eq 'Interpreter_(EvalInstruction|ExecuteInstruction)' \
            <<< "$dispatch_relocations"; then
            printf 'FAIL: %s still calls a per-opcode function in %s\n' \
                "$dispatch_loop" "$dispatch_object" >&2
            exit 1
        fi

        # Native invocation remains a deliberate separate boundary. Its
        # bookkeeping and diagnostic buffers must not inflate every opcode.
        if ! grep -q 'Interpreter_CallNative' <<< "$dispatch_relocations"; then
            printf 'FAIL: native helper boundary missing from %s in %s\n' \
                "$dispatch_loop" "$dispatch_object" >&2
            exit 1
        fi
    done
done

printf 'Interpreter dispatch code-generation tests passed (in-loop opcodes, retained single-step/native boundaries).\n'
