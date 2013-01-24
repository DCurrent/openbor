/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "depends.h"
#include "StackedSymbolTable.h"
#include "Instruction.h"
#include "Parser.h"

typedef HRESULT (*SCRIPTFUNCTION)(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

typedef struct Interpreter {
   StackedSymbolTable theSymbolTable;
   List* ptheFunctionList;               //external functionlist, to save some memory
   List theImportList;
   List theInstructionList;
   List paramList;
   Stack theDataStack;
   Stack theLabelStack;
   Parser theParser;
   pp_context theContext;

   Instruction** pCurrentInstruction;
   Instruction** pCurrentCall;
   Instruction** pReturnEntry;
   union { // we have to use the index before solidifying the instruction list
	  Instruction** pMainEntry;
	  int mainEntryIndex;
   };
   union {
	  Instruction** pClearEntry;
	  int clearEntryIndex;
   };
   union {
	  Instruction** pInitEntry;
	  int initEntryIndex;
   };
   int bHasImmediateCode;

   BOOL bCallCompleted;
   BOOL bMainCompleted;
   BOOL bReset; // 2011/11/13 UT: prevent nested call which is not supported by the script interpreter
}Interpreter;

void Interpreter_Init(Interpreter* pinterpreter, LPCSTR name, List* pflist);
void Interpreter_Clear(Interpreter* pinterpreter);
HRESULT Interpreter_ParseText(Interpreter* pinterpreter, LPSTR scriptText,
						   ULONG startingLineNumber, LPCSTR path);
HRESULT Interpreter_PutValue(Interpreter* pinterpreter, LPCSTR variable, ScriptVariant* pValue, int refFlag );
HRESULT Interpreter_GetValue(Interpreter* pinterpreter, LPCSTR variable, ScriptVariant* pValue);
HRESULT Interpreter_GetValueByRef(Interpreter* pinterpreter, LPCSTR variable, ScriptVariant** pValue);
HRESULT Interpreter_EvaluateImmediate(Interpreter* pinterpreter);
HRESULT Interpreter_EvaluateCall(Interpreter* pinterpreter);
HRESULT Interpreter_CompileInstructions(Interpreter* pinterpreter);
HRESULT Interpreter_Call(Interpreter* pinterpreter);
HRESULT Interpreter_EvalInstruction(Interpreter* pinterpreter);
void Interpreter_Reset(Interpreter* pinterpreter);


#endif
