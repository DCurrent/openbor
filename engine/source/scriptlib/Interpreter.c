/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include "../openborscript/config.h"
#include "Interpreter.h"
#include "ImportCache.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void Interpreter_Init(Interpreter *pinterpreter, LPCSTR name, List *pflist)
{
    memset(pinterpreter, 0, sizeof(Interpreter));
    StackedSymbolTable_Init(&(pinterpreter->theSymbolTable), name);
    Parser_Init(&(pinterpreter->theParser));
    pinterpreter->ptheFunctionList = pflist;
    List_Init(&(pinterpreter->theImportList));
    List_Init(&(pinterpreter->theInstructionList));
    List_Init(&(pinterpreter->paramList));
    Stack_Init(&(pinterpreter->theDataStack));
    Stack_Init(&(pinterpreter->theLabelStack));
    pp_context_init(&(pinterpreter->theContext));
}

void Interpreter_Clear(Interpreter *pinterpreter)
{
    int i, size;
    Instruction *pinstruction = NULL;
    ScriptVariant *pvariant = NULL;

    pp_context_destroy(&(pinterpreter->theContext));

    StackedSymbolTable_Clear(&(pinterpreter->theSymbolTable));
    Parser_Clear(&(pinterpreter->theParser));
    if(pinterpreter->theInstructionList.solidlist)
    {
        size = pinterpreter->theInstructionList.size;
        for(i = 0; i < size; i++)
        {
            Instruction_Clear(pinterpreter->theInstructionList.solidlist[i]);
            free((void *)pinterpreter->theInstructionList.solidlist[i]);
            pinterpreter->theInstructionList.solidlist[i] = NULL;
        }
    }
    else
    {
        FOREACH(
            pinterpreter->theInstructionList,
            pinstruction = (Instruction *)List_Retrieve(&(pinterpreter->theInstructionList));
            Instruction_Clear(pinstruction);
            free((void *)pinstruction);
            pinstruction = NULL;
        );
    }
    while(!Stack_IsEmpty(&(pinterpreter->theDataStack)))
    {
        pvariant = (ScriptVariant *)Stack_Top(&(pinterpreter->theDataStack));
        ScriptVariant_Clear(pvariant);
        free((void *)pvariant);
        Stack_Pop(&(pinterpreter->theDataStack));
    }
    List_Clear(&(pinterpreter->theLabelStack));
    List_Clear(&(pinterpreter->theInstructionList));
    List_Clear(&(pinterpreter->paramList));
    memset(pinterpreter, 0, sizeof(Interpreter));
}


/******************************************************************************
*  ParseText -- This method parses the text in scriptText into a string of
*               byte-codes for the interpreter to execute.
*  Parameters: scriptText -- an LPCSTR containing the script to be parsed.
*              startingLineNumber -- The line number the script starts on
*                                    (For use in HTML-based scripts)
*              dwSourceContext -- DWORD which contains a host provided context
*                                 for the script being parsed.
*  Returns: E_FAIL if parser errors found else S_OK
******************************************************************************/
HRESULT Interpreter_ParseText(Interpreter *pinterpreter, LPSTR scriptText,
                              ULONG startingLineNumber, LPCSTR path)
{

    //Parse the script
    Parser_ParseText(&(pinterpreter->theParser), &(pinterpreter->theContext),
                     &(pinterpreter->theInstructionList), scriptText, startingLineNumber, path );

    if(pinterpreter->theParser.errorFound)
    {
        return E_FAIL;
    }
    else
    {
        return S_OK;
    }
}

/******************************************************************************
*  PutValue -- This method copies the VARIANT in pValue into the symbol
*  designated by variable.
*  Parameters: variable -- a LPCSTR which denotes which symbol to copy this
*                          value into.
*              pValue -- a pointer to a ScriptVariant which contains the value
*                        to be copied into the symbol.
*  Returns: S_OK
*           E_INVALIDARG
*           E_FAIL
******************************************************************************/
HRESULT Interpreter_PutValue(Interpreter *pinterpreter, LPCSTR variable, ScriptVariant *pValue , int refFlag)
{
    HRESULT hr = E_FAIL;
    Instruction *pref = NULL;
    Symbol *pSymbol = NULL;
    //Check arguments
    if ((pValue == NULL) || (variable == NULL ))
    {
        hr = E_FAIL;
    }
    else
    {
        //Get the CSymbol that contains the VARIANT we need
        pref = (Instruction *)List_Retrieve(&(pinterpreter->theInstructionList));
        if(refFlag && pref->theRef)
        {
            *(pref->theRef) = *pValue;
            hr = S_OK;
        }
        else if (StackedSymbolTable_FindSymbol(&(pinterpreter->theSymbolTable), variable, &pSymbol ))
        {
            //Copy the valye from the argument to the CSymbol
            ScriptVariant_Copy(&(pSymbol->var), pValue);
            if(refFlag && pSymbol->theRef)
            {
                pref->theRef = pSymbol->theRef->theVal;
            }
            hr = S_OK;
        }
    }

    return hr;
}

/******************************************************************************
*  GetValue -- This method copies the VARIANT in the symbol designated by
*              variable into the ScriptVariant.
*  Parameters: variable -- a LPCSTR which denotes which symbol to copy this
*                          value from.
*              pValue -- a pointer to a ScriptVariant into which to copy the
*                        value.
*  Returns: S_OK
*           E_INVALIDARG
*           E_FAIL
******************************************************************************/
HRESULT Interpreter_GetValue(Interpreter *pinterpreter, LPCSTR variable, ScriptVariant *pValue)
{
    HRESULT hr = E_FAIL;

    //Get the CSymbol that contains the VARIANT we need
    Symbol *pSymbol = NULL;

    if (StackedSymbolTable_FindSymbol(&(pinterpreter->theSymbolTable), variable, &pSymbol ))
    {
        //Copy the value from the symbol to the result VARIANT
        ScriptVariant_Copy(pValue, &(pSymbol->var));
        hr = S_OK;
    }

    return hr;
}

HRESULT Interpreter_GetValueByRef(Interpreter *pinterpreter, LPCSTR variable, ScriptVariant **ppValue )
{
    HRESULT hr = E_FAIL;

    //Get the CSymbol that contains the VARIANT we need
    Symbol *pSymbol = NULL;

    if (StackedSymbolTable_FindSymbol(&(pinterpreter->theSymbolTable), variable, &pSymbol ))
    {
        //Copy the value from the symbol to the result VARIANT
        *ppValue = pSymbol->theRef->theVal;
        hr = S_OK;
    }

    return hr;
}

/******************************************************************************
* Caskey, Damon V.
* 2026-07-29 - Reworked original implementation
*
* Executes the CALL instruction selected by the interpreter's current
* instruction pointer.
*
* Calls may target a script or imported function through ptheJumpTarget, or a
* native function through functionRef. Cached parameter metadata is validated
* before either target is invoked.
*
* Nested calls temporarily replace pCurrentCall. The previous call context is
* restored through the common cleanup path regardless of success or failure.
*
* Returns S_OK when the target executes successfully or E_FAIL when the call
* instruction, parameter metadata, target, or target execution is invalid.
******************************************************************************/
HRESULT Interpreter_Call(Interpreter *pinterpreter) {

    HRESULT hr = E_FAIL;
    Instruction **previousCall;
    Instruction **pCurrentCall;
    Instruction *currentCall;
    ScriptVariant **parameters;
    ScriptVariant *parameter;
    ScriptVariant *pretvar;
    ScriptVariantStringView string_view;
    char conversion_buffer[SCRIPT_VARIANT_CONVERSION_BUFFER_LENGTH];
    int i;

    if(!pinterpreter) {
        return E_FAIL;
    }

    previousCall = pinterpreter->pCurrentCall;
    pCurrentCall = (Instruction **)pinterpreter->pCurrentInstruction;

    /*
    * The current instruction pointer must reference an
    * actual CALL instruction.
    */
    if(!pCurrentCall || !*pCurrentCall) {
        printf("Script runtime error: missing current call instruction.\n");

        goto endcall;
    }

    pinterpreter->pCurrentCall = pCurrentCall;
    currentCall = *pCurrentCall;

    /*
    * Every compiled call stores its generated parameter
    * count in theRef and its parameter references in
    * theRefList.
    */
    if(!currentCall->theRef
       || currentCall->theRef->vt != VT_INTEGER
       || currentCall->theRef->lVal < 0
       || !currentCall->theRefList) {
        printf("Script runtime error: invalid cached parameter metadata.\n");

        goto endcall;
    }

    /*
    * The parser produces the parameter count as an int
    * stored in a legacy integer ScriptVariant.
    */
    const int paramCount = (int)currentCall->theRef->lVal;

    /*
    * Solidification preserves the logical list size.
    * It must agree with the count cached on the CALL
    * instruction.
    */
    if(List_GetSize(currentCall->theRefList) != paramCount) {

        printf("Script runtime error: cached parameter count does not match reference list size.\n");

        goto endcall;
    }

    /*
    * Empty calls legitimately have no contiguous pointer
    * table. Nonempty calls require one.
    */
    if(paramCount > 0 && !currentCall->theRefList->solidlist) {
        printf("Script runtime error: missing solid parameter reference table.\n");

        goto endcall;
    }

    parameters = (ScriptVariant **)currentCall->theRefList->solidlist;

    /*
    * Script and imported functions execute through an
    * interpreter jump target.
    */
    if(currentCall->ptheJumpTarget) {
        pinterpreter->pCurrentInstruction = currentCall->ptheJumpTarget;

        hr = Interpreter_EvaluateCall(pinterpreter);
    
    } else if(currentCall->functionRef) {

        /*
        * Native functions receive the contiguous parameter
        * table, return storage, and validated parameter count.
        */

        pretvar = currentCall->theVal;

        hr = currentCall->functionRef(parameters, &pretvar, paramCount);

        if(FAILED(hr)) {
            
            /*
            * Resolve the native function's registered name
            * for diagnostic output.
            */
            List_Includes(pinterpreter->ptheFunctionList, currentCall->functionRef);

            printf("Script function '%s' returned an exception. See any preceding error messages above for details.\n", List_GetName(pinterpreter->ptheFunctionList));

            if(paramCount > 0) {
                printf(" parameters: ");

                for(i = 0; i < paramCount; i++) {
                    parameter = parameters[i];

                    /*
                    * Parameter references are validated
                    * during compilation, though this guard
                    * prevents diagnostic handling from
                    * dereferencing corrupted metadata.
                    */
                    if(!parameter) {
                        printf("<invalid>, ");
                        continue;
                    }

                    if(FAILED(ScriptVariant_GetStringView(
                        parameter,
                        conversion_buffer,
                        sizeof(conversion_buffer),
                        &string_view
                    ))) {
                        printf("<invalid or oversized>, ");
                        continue;
                    }

                    if(parameter->vt == VT_STR) {
                        printf(
                            "\"%.*s\", ",
                            (int)string_view.length,
                            string_view.string
                        );
                    
                    } else {
                        printf(
                            "%.*s, ",
                            (int)string_view.length,
                            string_view.string
                        );
                    }
                }

                printf("\n");
            }
        }

    } else {
        /*
        * A compiled CALL must have either an interpreter
        * jump target or a native function reference.
        */
        printf("Script runtime error: call instruction has no target.\n");

        goto endcall;
    }

    /*
    * Successful execution resumes at the CALL instruction.
    * The evaluator advances from there through its normal
    * instruction progression.
    */
    if(SUCCEEDED(hr)) {
        pinterpreter->pCurrentInstruction = pCurrentCall;
    }

endcall:

    /*
    * Restore the enclosing call context on every path.
    * The previous implementation skipped this restoration
    * when an error jumped directly to endcall.
    */
    pinterpreter->pCurrentCall = previousCall;

    /*
    * Report completion unless the interpreter is currently
    * performing a reset.
    */
    pinterpreter->bCallCompleted = !pinterpreter->bReset;

    return hr;
}


/******************************************************************************
*  EvaluateImmediate -- This method scans the instruction list and evaluates
*  any immediate code that may be present.  In the instruction list, immediate
*  code is preceeded by an IMMEDIATE instruction, and followed by a DEFERRED
*  instruction.
*  Parameters: none
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Interpreter_EvaluateImmediate(Interpreter *pinterpreter)
{
    BOOL bImmediate = FALSE;
    HRESULT hr = S_OK;
    Instruction *pInstruction = NULL;
    int size, index;

    if(pinterpreter->bHasImmediateCode)
    {
        //Run through all the instructions in the list, only executing those wrapped
        //by IMMEDIATE and DEFERRED.
        size = pinterpreter->theInstructionList.size;
        for(index = 0; index < size; index++)
        {
            pInstruction = (Instruction *)(pinterpreter->theInstructionList.solidlist[index]);
            //Check the current mode
            if (pInstruction->OpCode == IMMEDIATE)
            {
                bImmediate = TRUE;
            }
            else if (pInstruction->OpCode == DEFERRED)
            {
                bImmediate = FALSE;
            }

            //If the current mode is Immediate, then evaluate the instruction
            if (bImmediate)
            {
                pinterpreter->pCurrentInstruction = ((Instruction **)pinterpreter->theInstructionList.solidlist) + index;
                hr = Interpreter_EvalInstruction(pinterpreter);
            }
            //If we failed, then we need to break out of this loop
            if (FAILED(hr))
            {
                break;
            }
        }
    }

    //The "main" function isn't technically immediate code, but it is the well
    //known entry point into C code, so we call it as part of immediate execution.
    //Don't call "main" if it has already been called once.
    if (!pinterpreter->bMainCompleted)
    {
        if(pinterpreter->pMainEntry)
        {
            pinterpreter->pCurrentInstruction = pinterpreter->pMainEntry;
            hr = Interpreter_EvaluateCall(pinterpreter);
        }
        else
        {
            hr = E_FAIL;
        }
        if (SUCCEEDED(hr))
            //Set the m_bMainCompleted flag
        {
            pinterpreter->bMainCompleted = TRUE;
        }
        //else printf("error: %s\n", pinterpreter->theSymbolTable.name);
        pinterpreter->bCallCompleted = FALSE;
    }

    pinterpreter->bReset = FALSE;

    return hr;
}

/******************************************************************************
*  EvaluateCall -- This method evaluates the byte-code of a single call into
*  this interpreter.
*  Parameters: none
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Interpreter_EvaluateCall(Interpreter *pinterpreter)
{
    HRESULT hr = S_OK;
    //Evaluate instructions until an error occurs or until the m_bCallCompleted
    //flag is set to true.
    while( ( SUCCEEDED(hr) ) && ( !pinterpreter->bCallCompleted )) //pinterpreter->bReset &&
    {
        hr = Interpreter_EvalInstruction(pinterpreter);
    }
    return hr;
}


/******************************************************************************
*  UNARYOP -- The unary ops( +, -, !, etc. ) all work the same, so this macro
*  helps clean up the code.  It pops one value off the stack, applies the
*  specified operator to it, and pushes the result.
******************************************************************************/
//only 'compile', dont realy use the operator.
#define COMPILEUNARYOP \
	if (pinterpreter->theDataStack.size >= 1){ \
		pSVar1 = Stack_Top(&(pinterpreter->theDataStack)); \
		pInstruction->theRef = pSVar1;\
	} else hr = E_FAIL;

//else
//HandleRuntimeError( pInstruction, STACK_FAILURE, this );

#define UNARYOP(x) \
	x(pInstruction->theRef);


/******************************************************************************
*  BINARYOP -- The binary ops( +, -, <, ==, etc. ) all work the same, so this
*  macro helps clean up the code.  It pops two values off the stack, applies
*  the specified operator to them, and pushes the result.
******************************************************************************/
//only 'compile', dont realy use the operator
#define COMPILEBINARYOP \
	if (pinterpreter->theDataStack.size >= 2){ \
		pSVar2 = Stack_Top(&(pinterpreter->theDataStack)); \
		Stack_Pop(&(pinterpreter->theDataStack)); \
		pSVar1 = Stack_Top(&(pinterpreter->theDataStack)); \
		Stack_Pop(&(pinterpreter->theDataStack)); \
		pRetVal = (ScriptVariant*)malloc(sizeof(ScriptVariant));\
		ScriptVariant_Init(pRetVal);\
		Stack_Push(&(pinterpreter->theDataStack),(void*)pRetVal); \
		pInstruction->theVal = pRetVal;\
		pInstruction->theRef = pSVar1;\
		pInstruction->theRef2 = pSVar2;\
	} else hr = E_FAIL;

//else
//HandleRuntimeError( pInstruction, STACK_FAILURE, this );

#define BINARYOP(x)  \
	ScriptVariant_Copy(pInstruction->theVal, x(pInstruction->theRef, pInstruction->theRef2));

HRESULT Interpreter_CompileInstructions(Interpreter *pinterpreter)
{
    int i, j, t, size;
    Instruction *pInstruction = NULL;
    Token *pToken ;
    Symbol *pSymbol = NULL;
    LPCSTR pLabel = NULL;
    ScriptVariant *pSVar1 = NULL;
    ScriptVariant *pSVar2 = NULL;
    ScriptVariant *pRetVal = NULL;
    ImportNode *pImport = NULL;
    HRESULT hr = S_OK;

    // Import any scripts named in #import directives (parsed by the preprocessor)
    size = pinterpreter->theContext.imports.size;
    List_Reset(&(pinterpreter->theContext.imports));
    for(i = 0; i < size; i++)
    {
        pLabel = List_GetName(&(pinterpreter->theContext.imports));
        pImport = ImportCache_ImportFile(pLabel);
        if(pImport == NULL)
        {
            return E_FAIL;    // ImportCache should print out the error message
        }
        List_InsertAfter(&(pinterpreter->theImportList), pImport, pLabel);
        List_GotoNext(&(pinterpreter->theContext.imports));
    }

    // We are done appending to the script at this point, so free the preprocessor context
    pp_context_destroy(&(pinterpreter->theContext));

#ifdef USE_INDEX
    // it seems the list is used readonly here, so lets create an index for faster lookup
    List_CreateIndices(&(pinterpreter->theInstructionList));
#endif

    if(List_FindByName(&(pinterpreter->theInstructionList), "main"))
    {
        pinterpreter->mainEntryIndex = List_GetIndex(&(pinterpreter->theInstructionList));
    }
    else
    {
        pinterpreter->mainEntryIndex = -1;
    }
    if(List_FindByName(&(pinterpreter->theInstructionList), "ondestroy"))
    {
        pinterpreter->clearEntryIndex = List_GetIndex(&(pinterpreter->theInstructionList));
    }
    else
    {
        pinterpreter->clearEntryIndex = -1;
    }
    if(List_FindByName(&(pinterpreter->theInstructionList), "oncreate"))
    {
        pinterpreter->initEntryIndex = List_GetIndex(&(pinterpreter->theInstructionList));
    }
    else
    {
        pinterpreter->initEntryIndex = -1;
    }
    List_Reset(&(pinterpreter->theInstructionList));
    size = List_GetSize(&(pinterpreter->theInstructionList));
    for(i = 0; i < size; i++)
    {
        pInstruction = (Instruction *)List_Retrieve(&(pinterpreter->theInstructionList));

        //char pStr[256];
        //Instruction_ToString(pInstruction, pStr);
        //printf("%s\n", pStr);
        //The OpCode will tell us what operation to perform.
        switch( pInstruction->OpCode )
        {
            //Push a constant string
        case CONSTSTR:
            //Push a constant double
        case CONSTDBL:
            //Push a constant integer
        case CONSTINT:
            //convert to constant first
            if(FAILED(Instruction_ConvertConstant(pInstruction)))
            {
                hr = E_FAIL;
                break;
            }
            Instruction_NewData(pInstruction);
            Stack_Push(&(pinterpreter->theDataStack), (void *)pInstruction->theVal);
            break;

            //Load a value into the data stack
        case LOAD:
            hr = Interpreter_GetValueByRef(pinterpreter, pInstruction->theToken->theSource, &(pInstruction->theRef));
            // cache value
            Instruction_NewData(pInstruction);
            //push the value, not ref
            Stack_Push(&(pinterpreter->theDataStack), (void *)(pInstruction->theVal));
            break;

            //Save a value from the data stack
        case SAVE:
            pSVar2 = (ScriptVariant *)Stack_Top(&(pinterpreter->theDataStack));
            Stack_Pop(&(pinterpreter->theDataStack));
            hr = Interpreter_GetValueByRef(pinterpreter, pInstruction->theToken->theSource, &(pInstruction->theRef));
            pInstruction->theRef2 = pSVar2;
            break;

            //use the UNARYOP macro to do an AutoIncrement
        case INC:
            //use the UNARYOP macro to do an AutoDecrement
        case DEC:
            //use the UNARYOP macro to do a unary plus
        case POS:
            //use the UNARYOP macro to do a unary minus
        case NEG:
            //use the UNARYOP macro to do a logical not
        case NOT:
            //use the UNARYOP macro to do a bitwise not
        case BIT_NOT:
            COMPILEUNARYOP;
            break;

            //Use the BINARYOP macro to do a multiply
        case MUL:
            //Use the BINARYOP macro to do a divide
        case DIV:
            //Use the BINARYOP macro to do a mod
        case MOD:
            //Use the BINARYOP macro to do an add
        case ADD:
            //Use the BINARYOP macro to do a subtract
        case SUB:
            //Use the BINARYOP macro to do a left shift
        case SHL:
            //Use the BINARYOP macro to do a right shift
        case SHR:
            //Use the BINARYOP macro to do a greater than- equal
        case GE:
            //Use the BINARYOP macro to do a less than- equal
        case LE:
            //Use the BINARYOP macro to do a less than
        case LT:
            //Use the BINARYOP macro to do a greater than
        case GT:
            //Use the BINARYOP macro to do an equality
        case EQ:
            //Use the BINARYOP macro to do a not-equal
        case NE:
            //Use the BINARYOP macro to do a logical OR
        case OR:
            //Use the BINARYOP macro to do a logical AND
        case AND:
            //Use the BINARYOP macro to do a bitwise OR
        case BIT_OR:
            //Use the BINARYOP macro to do a bitwise XOR
        case XOR:
            //Use the BINARYOP macro to do a bitwise AND
        case BIT_AND:
            COMPILEBINARYOP;
            break;

            //Create a new CSymbol and add it to the symbol table
        case DATA:

            //Create a new CSymbol and add it to the symbol table
        case PARAM:
            Instruction_NewData(pInstruction); //cache the new variant
            pToken = pInstruction->theToken;
            pSymbol = (Symbol *)malloc(sizeof(Symbol));
            Symbol_Init(pSymbol, pToken->theSource, 0, NULL, pInstruction);
            StackedSymbolTable_AddSymbol(&(pinterpreter->theSymbolTable), pSymbol );
            break;

        //Call the specified method, and pass in a ScriptVariant* to receive the
        //return value.  If it's not NULL, then push it onto the data stack.
        case CALL:
        {
            /*
            * Preserve the CALL instruction's label. Function
            * resolution may move the instruction-list cursor.
            */
            pLabel = List_GetName(
                &(pinterpreter->theInstructionList));

            pToken = pInstruction->theToken;
            pInstruction->functionRef = NULL;

            /*
            * Resolve the call target. Calls may reference a
            * script function, imported function, or native
            * system function.
            */
            if(List_FindByName(&(pinterpreter->theInstructionList), pToken->theSource)) {

                pInstruction->theJumpTargetIndex = List_GetIndex(&(pinterpreter->theInstructionList));

                pInstruction->jumpTargetType = 1;

                /*
                * Function lookup selected the target instruction.
                * Restore the CALL instruction as the current node.
                */
                List_FindByName(&(pinterpreter->theInstructionList), pLabel);

            } else if(ImportList_GetFunctionPointer(&(pinterpreter->theImportList), pToken->theSource)) {

                pInstruction->ptheJumpTarget = ImportList_GetFunctionPointer(&(pinterpreter->theImportList), pToken->theSource);

                assert((size_t)pInstruction->ptheJumpTarget >= size);

            } else if(List_FindByName(pinterpreter->ptheFunctionList, pToken->theSource)) {

                pInstruction->functionRef = (SCRIPTFUNCTION)List_Retrieve(pinterpreter->ptheFunctionList);

            } else {

                /*
                * No script, imported, or native function matched
                * the requested name. Compilation cannot produce a
                * valid call target.
                */
                printf("Script compile error: can't find function '%s'.\n", pToken->theSource);

                hr = E_FAIL;
                break;
            }

            /*
            * The parser pushes a generated argument count after
            * pushing the argument expressions. An empty stack
            * indicates malformed or incomplete call bytecode.
            */
            if(Stack_IsEmpty(&(pinterpreter->theDataStack))) {

                printf("Script compile error: missing parameter count for function '%s'.\n", pToken->theSource);

                hr = E_FAIL;
                break;
            }

            pSVar1 = (ScriptVariant *)Stack_Top(&(pinterpreter->theDataStack));

            /*
            * Parameter counts originate from the parser and must
            * be nonnegative legacy integers. Validate the cached
            * value before removing it from the compilation stack.
            */
            if(!pSVar1
                || pSVar1->vt != VT_INTEGER
                || pSVar1->lVal < 0) {

                printf("Script compile error: invalid parameter count for function '%s'.\n", pToken->theSource);

                hr = E_FAIL;
                break;
            }

            /*
            * Cache the validated count reference on the CALL
            * instruction. Stack removal does not destroy the
            * referenced ScriptVariant.
            */
            Stack_Pop(&(pinterpreter->theDataStack));
            pInstruction->theRef = pSVar1;

            /*
            * The parser maintains its count as an int, so the
            * generated legacy integer is safe to narrow here.
            */
            const int paramCount = (int)pSVar1->lVal;

            /*
            * Each CALL owns a dynamically sized list of argument
            * references. No fixed parameter capacity is required.
            */
            pInstruction->theRefList = malloc(sizeof(*pInstruction->theRefList));

            if(!pInstruction->theRefList) {

                /*
                * Without the reference list, the runtime could not
                * pass arguments to the resolved function.
                */
                printf("Script compile error: unable to allocate parameter reference list for function '%s'.\n", pToken->theSource);

                hr = E_FAIL;
                break;
            }

            List_Init(pInstruction->theRefList);

            /*
            * Argument instructions were emitted in reverse order.
            * Removing their references from the compilation stack
            * restores normal source order in the call list.
            */
            for(j = 0; j < paramCount; j++) {

                /*
                * Reaching an empty stack before collecting the
                * declared number of arguments indicates an
                * inconsistent parser or instruction state.
                */
                if(Stack_IsEmpty(&(pinterpreter->theDataStack))) {

                    printf("Script compile error: parameter stack underflow for function '%s'.\n", pToken->theSource);

                    hr = E_FAIL;
                    break;
                }

                pSVar2 = (ScriptVariant *)Stack_Top(&(pinterpreter->theDataStack));

                /*
                * A populated stack must reference an actual
                * ScriptVariant for each cached argument.
                */
                if(!pSVar2) {

                    printf("Script compile error: invalid parameter reference for function '%s'.\n", pToken->theSource);

                    hr = E_FAIL;
                    break;
                }

                Stack_Pop(&(pinterpreter->theDataStack));

                List_InsertAfter(pInstruction->theRefList, pSVar2, NULL);
            }

            /*
            * Parameter retrieval may have left a partially built
            * reference list. Stop before exposing it to later
            * compilation stages.
            */
            if(FAILED(hr)) {
                break;
            }

            /*
            * Convert the linked reference list into the contiguous
            * pointer table consumed by native and script calls.
            */
            const bool solidified = List_Solidify(pInstruction->theRefList);

            if(!solidified) {

                /*
                * Solidification failure means the contiguous
                * argument table could not be allocated.
                */
                printf("Script compile error: unable to solidify parameter reference list for function '%s'.\n", pToken->theSource);

                hr = E_FAIL;
                break;
            }

            /*
            * Allocate and cache storage for the function's
            * eventual return value.
            */
            Instruction_NewData(pInstruction);

            /*
            * Translate eligible string constants while argument
            * references and call metadata are fully available.
            */
            if(!Script_MapStringConstants(pInstruction)) {
                hr = E_FAIL;
                break;
            }

            /*
            * Inspect the instruction following CALL. Its presence
            * is required to determine whether the return value is
            * consumed or explicitly discarded.
            */
            const bool hasNext = List_GotoNext(&(pinterpreter->theInstructionList));

            if(!hasNext) {

                printf("Script compile error: missing instruction after call to function '%s'.\n", pToken->theSource);

                hr = E_FAIL;
                break;
            }

            Instruction *nextInstruction = (Instruction *)List_Retrieve(&(pinterpreter->theInstructionList));

            if(!nextInstruction) {

                /*
                * Navigation succeeded but produced no instruction.
                * Restore the CALL node before reporting failure.
                */
                List_GotoPrevious(&(pinterpreter->theInstructionList));

                printf("Script compile error: invalid instruction after call to function '%s'.\n", pToken->theSource);

                hr = E_FAIL;
                break;
            }

            const bool isNextInstructionClean = nextInstruction->OpCode == CLEAN;

            /*
            * CLEAN explicitly discards the function result.
            * Otherwise, place the cached result on the data stack
            * for use by the surrounding expression.
            */
            if(!isNextInstructionClean) {
                Stack_Push(&(pinterpreter->theDataStack), pInstruction->theVal);
            }

            /*
            * Following-instruction inspection moved the list
            * cursor forward. Restore the CALL instruction before
            * continuing compilation.
            */
            List_GotoPrevious(&(pinterpreter->theInstructionList));

            break;
        }

        //Jump to the specified label
        case JUMP:
            pLabel = pInstruction->Label;
            //cache the jump target
            if(List_FindByName(&(pinterpreter->theInstructionList), pLabel))
            {
                pInstruction->theJumpTargetIndex = List_GetIndex(&(pinterpreter->theInstructionList));
                pInstruction->jumpTargetType = 1;
                List_Includes(&(pinterpreter->theInstructionList), pInstruction); // hop back
            }
            else
            {
                hr = E_FAIL;
            }
            break;

            //Pop an entry from the data stack and jump to the specified label
        case PJUMP:
            Stack_Pop(&(pinterpreter->theDataStack));
            pLabel = pInstruction->Label;
            //cache the jump target
            if(List_FindByName(&(pinterpreter->theInstructionList), pLabel))
            {
                pInstruction->theJumpTargetIndex = List_GetIndex(&(pinterpreter->theInstructionList));
                pInstruction->jumpTargetType = 1;
                List_Includes(&(pinterpreter->theInstructionList), pInstruction); // hop back
            }
            else
            {
                hr = E_FAIL;
            }
            break;

            //Jump to the end of function, infact it is return
        case JUMPR:
            pSVar1 = (ScriptVariant *)Stack_Top(&(pinterpreter->theDataStack));
            Stack_Pop(&(pinterpreter->theDataStack));
            // cache the return value
            pInstruction->theRef = pSVar1;
            pLabel = pInstruction->Label;
            //cache the jump target
            if(List_FindByName(&(pinterpreter->theInstructionList), pLabel))
            {
                pInstruction->theJumpTargetIndex = List_GetIndex(&(pinterpreter->theInstructionList));
                pInstruction->jumpTargetType = 1;
                List_Includes(&(pinterpreter->theInstructionList), pInstruction); // hop back
            }
            else
            {
                hr = E_FAIL;
            }
            break;

            //Jump if the top two ScriptVariants are equal, but only pop the topmost one
        case Branch_EQUAL:
            pLabel = pInstruction->Label;
            if (pinterpreter->theDataStack.size >= 2)
            {
                pInstruction->theRef2 = Stack_Top(&(pinterpreter->theDataStack));
                Stack_Pop(&(pinterpreter->theDataStack));
                pInstruction->theRef = Stack_Top(&(pinterpreter->theDataStack));
                //note that we do *not* pop the second value from the stack
            }
            else
            {
                hr = E_FAIL;
            }
            if(List_FindByName(&(pinterpreter->theInstructionList), pLabel))
            {
                pInstruction->theJumpTargetIndex = List_GetIndex(&(pinterpreter->theInstructionList));
                pInstruction->jumpTargetType = 1;
                List_Includes(&(pinterpreter->theInstructionList), pInstruction); // hop back
            }
            else
            {
                hr = E_FAIL;
            }
            break;

            //Jump if the top ScriptVariant resolves to false
        case Branch_FALSE:
            //Jump if the top ScriptVariant resolves to true
        case Branch_TRUE:
            //cache the reference target
            pLabel = pInstruction->Label;
            pSVar1 = Stack_Top(&(pinterpreter->theDataStack));
            pInstruction->theRef = pSVar1;
            Stack_Pop(&(pinterpreter->theDataStack));
            //cache the jump target
            if(List_FindByName(&(pinterpreter->theInstructionList), pLabel))
            {
                pInstruction->theJumpTargetIndex = List_GetIndex(&(pinterpreter->theInstructionList));
                pInstruction->jumpTargetType = 1;
                List_Includes(&(pinterpreter->theInstructionList), pInstruction); // hop back
            }
            else
            {
                hr = E_FAIL;
            }
            break;

            //Set the m_bCallCompleted flag to true so we know to stop evalutating
            //instructions
        case RET:
            break;

            //Make sure the argument count on the top of the stack matches the
            //number of arguments we have
        case CHECKARG:
            //cache the argument count
            if(FAILED(Instruction_ConvertConstant(pInstruction)))
            {
                hr = E_FAIL;
            }
            break;

            //This instructs the interpreter to clean one value off the stack.
        case CLEAN:
            Stack_Pop(&(pinterpreter->theDataStack));
            break;

            //This OpCode denotes an error in the instruction list
        case ERR:
            hr = E_FAIL;
            break;

            //This is a placeholder.  Don't do anything.
        case NOOP:
        case FUNCDECL:
            break;

            //This instructs the interpreter to push a symbol scope.
        case PUSH:
            StackedSymbolTable_PushScope(&(pinterpreter->theSymbolTable), NULL);
            break;

            //This instructs the interpreter to pop a symbol scope
        case POP:
            StackedSymbolTable_PopScope(&(pinterpreter->theSymbolTable));
            break;

            //Ignore IMMEDIATE and DEFFERRED instructions, since they are only used
            //to mark immediate code.
        case IMMEDIATE:
            pinterpreter->bHasImmediateCode = TRUE;
            break;
        case DEFERRED:
            break;

            //If we hit the default, then we got an unrecognized instruction
        default:
            hr = E_FAIL;

            //Report an error
            //HandleRuntimeError( pInstruction, INVALID_INSTRUCTION, this );
            break;
        }
        // if we fail, go back to normal, clear up the compile productions
        if(FAILED(hr))
        {
            printf("\nScript compile error in '%s': %s line %d, column %d\n",
                   pinterpreter->theSymbolTable.name, (pInstruction->theToken) ? pInstruction->theToken->theSource : "",
                   (pInstruction->theToken) ? pInstruction->theToken->theTextPosition.row : -1,
                   (pInstruction->theToken) ? pInstruction->theToken->theTextPosition.col : -1);
            List_Reset(&(pinterpreter->theInstructionList));
            for(i = 0; i < size; i++)
            {
                pInstruction = (Instruction *)List_Retrieve(&(pinterpreter->theInstructionList));
                if(pInstruction->theVal)
                {
                    ScriptVariant_Clear(pInstruction->theVal);
                    free(pInstruction->theVal);
                    pInstruction->theVal = NULL;
                }
                if(pInstruction->theRefList)
                {
                    List_Clear(pInstruction->theRefList);
                    free(pInstruction->theRefList);
                }
                pInstruction->theRef = pInstruction->theRef2 = NULL;
                List_GotoNext(&(pinterpreter->theInstructionList));
            }
            List_Clear(&(pinterpreter->theInstructionList));
            pinterpreter->mainEntryIndex = -1;
            pinterpreter->clearEntryIndex = -1;
            pinterpreter->initEntryIndex = -1;
            break;
        }
        List_GotoNext(&(pinterpreter->theInstructionList));
    }
    // clear some unused properties
    List_Reset(&(pinterpreter->theInstructionList));
    size = List_GetSize(&(pinterpreter->theInstructionList));
    for(i = 0; i < size; i++)
    {
        pInstruction = (Instruction *)List_Retrieve(&(pinterpreter->theInstructionList));
        // we might not need these 2, free them to cut some memory usage
        if(pInstruction->theToken)
        {
            free(pInstruction->theToken);
            pInstruction->theToken = NULL;
        }
        if(pInstruction->Label)
        {
            free(pInstruction->Label);
            pInstruction->Label = NULL;
        }
        List_GotoNext(&(pinterpreter->theInstructionList));
    }

    // clear the import list since we no longer need it
    List_Clear(&(pinterpreter->theImportList));

    // make a solid list that can be referenced by index
    List_Solidify(&(pinterpreter->theInstructionList));
    StackedSymbolTable_Clear(&(pinterpreter->theSymbolTable));
    List_Clear(&(pinterpreter->theDataStack));
    List_Clear(&(pinterpreter->theLabelStack));

    // convert mainEntryIndex (int) to pMainEntry (Instruction**)
    if(pinterpreter->mainEntryIndex >= 0)
    {
        pinterpreter->pMainEntry = (Instruction **)(&(pinterpreter->theInstructionList.solidlist[pinterpreter->mainEntryIndex]));
    }
    else
    {
        pinterpreter->pMainEntry = NULL;
    }

    if(pinterpreter->clearEntryIndex >= 0)
    {
        pinterpreter->pClearEntry = (Instruction **)(&(pinterpreter->theInstructionList.solidlist[pinterpreter->clearEntryIndex]));
    }
    else
    {
        pinterpreter->pClearEntry = NULL;
    }

    if(pinterpreter->initEntryIndex >= 0)
    {
        pinterpreter->pInitEntry = (Instruction **)(&(pinterpreter->theInstructionList.solidlist[pinterpreter->initEntryIndex]));
    }
    else
    {
        pinterpreter->pInitEntry = NULL;
    }

    // convert theJumpTargetIndex (int) to ptheJumpTarget (Instruction**)
    for(i = 0; i < size; i++)
    {
        pInstruction = (Instruction *)(pinterpreter->theInstructionList.solidlist[i]);
        pInstruction->step = 1;
        if(i < size - 1 && !pInstruction->jumpTargetType && !pInstruction->theJumpTargetIndex)
        {
            for(j = i; j < size - 1; j++)
            {
                switch(((Instruction *)(pinterpreter->theInstructionList.solidlist[j + 1]))->OpCode)
                {
                case DATA:
                case CLEAN:
                case NOOP:
                case FUNCDECL:
                case PUSH:
                case POP:
                case CONSTINT:
                case CONSTDBL:
                case CONSTSTR:
                case CHECKARG:
                    pInstruction->step++;
                    break;
                default:
                    j = size;
                    break;
                }
            }
        }
        if(pInstruction->jumpTargetType && pInstruction->theJumpTargetIndex >= 0 && pInstruction->theJumpTargetIndex < size)
        {
            t = pInstruction->theJumpTargetIndex;
            pInstruction->ptheJumpTarget = (Instruction **) & (pinterpreter->theInstructionList.solidlist[pInstruction->theJumpTargetIndex]);
            //jump targets are always placeholders, so skip those opcode that does nothing
            for(j = t; j < size - 1; j++)
            {
                switch(pInstruction->ptheJumpTarget[1]->OpCode)
                {
                case DATA:
                case CLEAN:
                case NOOP:
                case FUNCDECL:
                case PUSH:
                case POP:
                case CONSTINT:
                case CONSTDBL:
                case CONSTSTR:
                case CHECKARG:
                    pInstruction->ptheJumpTarget++;
                    break;
                default:
                    j = size;
                    break;
                }
            }
        }
    }

    return hr;
}


/******************************************************************************
*  EvalInstruction -- This method evaluates a single byte-code instruction.
*  Parameters: none
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Interpreter_EvalInstruction(Interpreter *pinterpreter)
{
    HRESULT hr = S_OK;
    //Retrieve the current instruction from the list
    Instruction *pInstruction;
    Instruction *currentCall;
    Instruction *returnEntry;

    if((pInstruction = *((Instruction **)pinterpreter->pCurrentInstruction)))
    {

        //The OpCode will tell us what operation to perform.
        switch( pInstruction->OpCode )
        {
            //Push a constant string
        case CONSTSTR:
            //Push a constant double
        case CONSTDBL:
            //Push a constant integer
        case CONSTINT:
            break;

            //Load a value into the data stack
        case LOAD:
            ScriptVariant_Copy(pInstruction->theVal, pInstruction->theRef);
            break;

            //Save a value from the data stack
        case SAVE:
            ScriptVariant_Copy(pInstruction->theRef, pInstruction->theRef2);
            break;

            //use the UNARYOP macro to do an AutoIncrement
        case INC:
            UNARYOP(ScriptVariant_Inc_Op);
            break;

            //use the UNARYOP macro to do an AutoDecrement
        case DEC:
            UNARYOP(ScriptVariant_Dec_Op);
            break;

            //use the UNARYOP macro to do a unary plus
        case POS:
            UNARYOP(ScriptVariant_Pos);
            break;

            //use the UNARYOP macro to do a unary minus
        case NEG:
            UNARYOP(ScriptVariant_Neg);
            break;

            //use the UNARYOP macro to do a logical not
        case NOT:
            UNARYOP(ScriptVariant_Boolean_Not);
            break;

            //use the UNARYOP macro to do a bitwise not
        case BIT_NOT:
            UNARYOP(ScriptVariant_Bitwise_Not);
            break;


            //Use the BINARYOP macro to do a multipy
        case MUL:
            BINARYOP(ScriptVariant_Mul);
            break;

            //Use the BINARYOP macro to do a divide
        case DIV:
            BINARYOP(ScriptVariant_Div);
            break;

            //Use the BINARYOP macro to do a mod
        case MOD:
            BINARYOP(ScriptVariant_Mod);
            break;

            //Use the BINARYOP macro to do an add
        case ADD:
            BINARYOP(ScriptVariant_Add);
            break;

            //Use the BINARYOP macro to do a subtract
        case SUB:
            BINARYOP(ScriptVariant_Sub);
            break;

            //Use the BINARYOP macro to do a left shift
        case SHL:
            BINARYOP(ScriptVariant_Shl);
            break;

            //Use the BINARYOP macro to do a right shift
        case SHR:
            BINARYOP(ScriptVariant_Shr);
            break;

            //Use the BINARYOP macro to do a greater than- equal
        case GE:
            BINARYOP(ScriptVariant_Ge);
            break;

            //Use the BINARYOP macro to do a less than- equal
        case LE:
            BINARYOP(ScriptVariant_Le);
            break;

            //Use the BINARYOP macro to do a less than
        case LT:
            BINARYOP(ScriptVariant_Lt);
            break;

            //Use the BINARYOP macro to do a greater than
        case GT:
            BINARYOP(ScriptVariant_Gt);
            break;

            //Use the BINARYOP macro to do an equality
        case EQ:
            BINARYOP(ScriptVariant_Eq);
            break;

            //Use the BINARYOP macro to do a not-equal
        case NE:
            BINARYOP(ScriptVariant_Ne);
            break;

            //Use the BINARYOP macro to do a logical OR
        case OR:
            BINARYOP(ScriptVariant_Or);
            break;

            //Use the BINARYOP macro to do a logical AND
        case AND:
            BINARYOP(ScriptVariant_And);
            break;

            //Use the BINARYOP macro to do a bitwise OR
        case BIT_OR:
            BINARYOP(ScriptVariant_Bit_Or);
            break;

            //Use the BINARYOP macro to do a bitwise XOR
        case XOR:
            BINARYOP(ScriptVariant_Xor);
            break;

            //Use the BINARYOP macro to do a bitwise AND
        case BIT_AND:
            BINARYOP(ScriptVariant_Bit_And);
            break;

            //Create a new CSymbol and add it to the symbol table
        case DATA:
            //again, we dont have to define anything if compiled
            break;

            //Create a new CSymbol from a value on the stack and add it to the
            //symbol table.
        case PARAM:
            //copy value from the cached parameter
            //assert(pinterpreter->pCurrentCall);
            currentCall = *(pinterpreter->pCurrentCall);
            if(List_GetSize(currentCall->theRefList) <= currentCall->theRefList->index)
            {
                ScriptVariant_Clear(pInstruction->theVal);
            }
            else
            {
                ScriptVariant_Copy(pInstruction->theVal, (ScriptVariant *)(currentCall->theRefList->solidlist[currentCall->theRefList->index]));
            }
            currentCall->theRefList->index++;
            break;

            //Call the specified method, and pass in a ScriptVariant* to receive the
            //return value.  If it's not NULL, then push it onto the data stack.
        case CALL:
            pInstruction->theRefList->index = 0;
            hr = Interpreter_Call(pinterpreter);
            //Reset the m_bCallCompleted flag back to false
            //pinterpreter->bCallCompleted = FALSE;
            break;

            // return
        case JUMPR:
            pinterpreter->pReturnEntry = pinterpreter->pCurrentInstruction;
            pinterpreter->pCurrentInstruction = pInstruction->ptheJumpTarget;
            break;

            //Jump to the specified label
        case JUMP:
        case PJUMP:
            pinterpreter->pCurrentInstruction = pInstruction->ptheJumpTarget;
            break;

            //Jump if the top two ScriptVariants are equal
        case Branch_EQUAL:
            if(ScriptVariant_Eq(pInstruction->theRef, pInstruction->theRef2)->lVal)
            {
                pinterpreter->pCurrentInstruction = pInstruction->ptheJumpTarget;
            }
            break;

            //Jump if the top ScriptVariant resolves to false
        case Branch_FALSE:
            if(!ScriptVariant_IsTrue(pInstruction->theRef))
            {
                pinterpreter->pCurrentInstruction = pInstruction->ptheJumpTarget;
            }
            break;

            //Jump if the top ScriptVariant resolves to true
        case Branch_TRUE:
            if(ScriptVariant_IsTrue(pInstruction->theRef))
            {
                pinterpreter->pCurrentInstruction = pInstruction->ptheJumpTarget;
            }
            break;

            //Set the m_bCallCompleted flag to true so we know to stop evalutating
            //instructions
        case RET:
            if(pinterpreter->pReturnEntry)
            {
                returnEntry = *(pinterpreter->pReturnEntry);
                if(returnEntry->theRef && pinterpreter->pCurrentCall)
                {
                    currentCall = *(pinterpreter->pCurrentCall);
                    ScriptVariant_Copy(currentCall->theVal, returnEntry->theRef);
                }
            }
            pinterpreter->pReturnEntry = NULL;
            pinterpreter->bCallCompleted = TRUE;
            break;

            //Make sure the argument count on the top of the stack matches the
            //number of arguments we have
        case CHECKARG:
            /*
            if(pinterpreter->pCurrentCall)
            {
            	currentCall = *(pinterpreter->pCurrentCall);
            	if(pInstruction->theVal->lVal != currentCall->theRef->lVal)
            	{
            		printf("Runtime error: argument count(%d) doesn't match, check your function call: %s.\n", (int)pInstruction->theVal->lVal, currentCall->Label);
            		hr = E_FAIL;
            	}
            }*/
            break;

            //This instructs the interpreter to clean one value off the stack.
        case CLEAN:
            break;

            //This OpCode denotes an error in the instruction list
        case ERR:
            hr = E_FAIL;
            break;

            //This is a placeholder.  Don't do anything.
        case NOOP:
        case FUNCDECL:
            break;

            //This instructs the interpreter to push a symbol scope.
        case PUSH:
            break;

            //This instructs the interpreter to pop a symbol scope
        case POP:
            break;

            //Ignore IMMEDIATE and DEFFERRED instructions, since they are only used
            //to mark immediate code.
        case IMMEDIATE:
        case DEFERRED:
            break;

            //If we hit the default, then we got an unrecognized instruction
        default:
            printf("\nUn-handled OpCode: %d\n", pInstruction->OpCode);
            hr = E_FAIL;

            //Report an error
            //HandleRuntimeError( pInstruction, INVALID_INSTRUCTION, this );
            break;
        }
        //Increment the instruction list
        pinterpreter->pCurrentInstruction += pInstruction->step;
    }

    //return the result of this token evaluation
    return hr;
}

/******************************************************************************
*  OutputPCode -- This method creates a new text file called <fileName>.txt
*                 that contains a pseudo-assembly representation of the PCode
*                 stored in this CInterpreter object.  The file cannot be read
*                 back into the interpreter.  It is created for debugging
*                 purposes only.
*  Parameters: fileName -- an LPCSTR containing the name of the file to
*                          store the pseudo-assembly code in.  An LPCSTR is
*                          used for consistency with the rest of the engine.
*  Returns: none
******************************************************************************/
void Interpreter_OutputPCode(Interpreter *pinterpreter, LPCSTR fileName )
{
    FILE *instStream = NULL;
    Instruction *pInstruction = NULL;
    LPCSTR pLabel = NULL;
    int i, size;
    //Declare and initialize some string buffers.
    static char buffer[256];
    static char pStr[256];

    //If the fileName is "", then substitute "Main".
    if (!strcmp( fileName, "" ))
    {
        strcpy( buffer, "Main" );
    }
    else
    {
        strcpy(buffer, fileName);
    }

    //Add ".txt" to the end of the names so the files get created right
    strcat( buffer, ".txt" );

    //Open an output stream for the productions enumeration.
    instStream = fopen(buffer, "w");
    if (!instStream)
    {
        return;
    }

    //Iterate through the list of instructions
    FOREACH( pinterpreter->theInstructionList,
             //Get the next CInstruction
             pInstruction = (Instruction *)List_Retrieve(&(pinterpreter->theInstructionList));

             //Re-initialize the buffers
             memset(buffer, 0, 256 );

             //If this instruction has a label, then get it and write it to the output
             //buffer
             pLabel = List_GetName(&(pinterpreter->theInstructionList));
             if (pLabel != NULL)
             strcpy(buffer, pLabel);
             strcat( buffer, "\t" );

             //Get the pseudo-assembly representation of the CInstruction and concat
             //it onto the output buffer
             Instruction_ToString(pInstruction, pStr);
             strcat( buffer, pStr );

             strcat( buffer, "\n");

             //Write the output buffer to the output stream
             fprintf(instStream, "%s", buffer);
           );
    //Close the output stream
    fclose(instStream);
}

/******************************************************************************
*  Reset -- This method resets the interpreter.
*  Parameters: none
*  Returns: none
******************************************************************************/
void Interpreter_Reset(Interpreter *pinterpreter)
{
    pinterpreter->pCurrentCall = NULL;
    pinterpreter->pReturnEntry = NULL;
    pinterpreter->pCurrentInstruction = NULL;
    //Reset the main flag
    pinterpreter->bMainCompleted = FALSE;
    pinterpreter->bReset = TRUE;
    pinterpreter->bCallCompleted = FALSE;
}
