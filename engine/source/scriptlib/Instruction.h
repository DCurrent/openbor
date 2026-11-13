/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef INSTRUCTION_H
#define INSTRUCTION_H
#include "depends.h"
#include "Lexer.h"
#include "List.h"
#include "ScriptVariant.h"

typedef LPSTR Label;

typedef enum OpCode { CONSTSTR, CONSTDBL, CONSTINT, LOAD, SAVE, INC, DEC, FIELD, CALL, POS, NEG,
                      NOT, BIT_NOT, MUL, DIV, MOD, ERR, ADD, SUB, SHL, SHR, JUMP, PJUMP, GE, LE, LT, GT, EQ, NE, OR, AND,
                      BIT_OR, XOR, BIT_AND, NOOP, PUSH, POP, Branch_FALSE, Branch_TRUE, Branch_EQUAL, DATA, PARAM,
                      IMMEDIATE, DEFERRED, RET, CHECKARG, CLEAN, JUMPR, FUNCDECL, CALL_NATIVE, OPCODE_END
                    } OpCode;

#pragma pack(4)

/*
* Damon V. Caskey
* 2026-11-12
*
* Retain execution-ready metadata for a resolved native call. Instructions
* own these descriptors; every referenced argument, result, and list cursor
* remains owned by the existing compiled instruction/value storage. Argument
* values are deliberately not cached: the same slots change on every call.
*/
typedef struct NativeCall
{
    ScriptVariant **parameters;
    ScriptVariant *result;
    HRESULT (*function)(ScriptVariant **, ScriptVariant **, int);
    int *parameter_index;
    int parameter_count;
    unsigned profile_native_id;
} NativeCall;

typedef struct Instruction
{
    unsigned OpCode;
    unsigned jumpTargetType;
    unsigned step;
    /* Stable diagnostic ID survives compile-time token disposal. Zero is
     * the profiler's unknown/overflow bucket, not a callable target. */
    unsigned profile_native_id;
    Token *theToken;
    CHAR *Label;//[MAX_STR_LEN+1];
    ScriptVariant *theVal;
    ScriptVariant *theRef;
    /* CALL has no second operand. Reuse that pointer-sized slot for its
     * optional native descriptor without enlarging other instructions. */
    union
    {
        ScriptVariant *theRef2;
        NativeCall *nativeCall;
    };
    List *theRefList;
    HRESULT (*functionRef)(ScriptVariant **, ScriptVariant **, int);
    union
    {
        int theJumpTargetIndex;
        struct Instruction **ptheJumpTarget;
        //struct Instruction* theJumpTarget;
    };
} Instruction;

#pragma pack()

HRESULT Instruction_InitViaToken(Instruction *pins, OpCode code, Token *pToken );
void Instruction_InitViaLabel(Instruction *pins, OpCode code, LPCSTR label );
void Instruction_Init(Instruction *pins);
void Instruction_Clear(Instruction *pins);
bool Instruction_CompileNativeCall(Instruction *pins);

void Instruction_NewData(Instruction *pins);
HRESULT Instruction_ConvertConstant(Instruction *pins);

void Instruction_ToString(Instruction *pins, LPSTR strRep);
#endif
