/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "depends.h"
#include "StackedSymbolTable.h"
#include "Instruction.h"
#include "Parser.h"

typedef ptrdiff_t (*SCRIPTFUNCTION)(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

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
   int bHasImmediateCode;

   BOOL bCallCompleted;
   BOOL bMainCompleted;
}Interpreter;

void Interpreter_Init(Interpreter* pinterpreter, char* name, List* pflist);
void Interpreter_Clear(Interpreter* pinterpreter);
ptrdiff_t Interpreter_ParseText(Interpreter* pinterpreter, char* scriptText,
						   ULONG startingLineNumber, char* path);
ptrdiff_t Interpreter_PutValue(Interpreter* pinterpreter, char* variable, ScriptVariant* pValue, int refFlag );
ptrdiff_t Interpreter_GetValue(Interpreter* pinterpreter, char* variable, ScriptVariant* pValue);
ptrdiff_t Interpreter_GetValueByRef(Interpreter* pinterpreter, char* variable, ScriptVariant** pValue);
ptrdiff_t Interpreter_EvaluateImmediate(Interpreter* pinterpreter);
ptrdiff_t Interpreter_EvaluateCall(Interpreter* pinterpreter);
ptrdiff_t Interpreter_CompileInstructions(Interpreter* pinterpreter);
ptrdiff_t Interpreter_Call(Interpreter* pinterpreter);
ptrdiff_t Interpreter_EvalInstruction(Interpreter* pinterpreter);
void Interpreter_Reset(Interpreter* pinterpreter);
void Interpreter_ClearImports(Interpreter* pinterpreter);


#endif
