/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "StackedSymbolTable.h"
#include <stdio.h>
/******************************************************************************
*  CStackedSymbolTable -- This class handles the scoping issues related to
*  the symbol table, allowing correct resolution of variable references.
******************************************************************************/

/******************************************************************************
*  Initialize -- This method intializes the stacked symbol table with its name
*  and creates a new stack for the symbol tables.
******************************************************************************/
void StackedSymbolTable_Init(StackedSymbolTable *sstable, LPCSTR theName )
{

    if (theName)
    {
        strcpy(sstable->name, theName);
    }
    else
    {
        sstable->name[0] = 0;
    }

    List_Init(&(sstable->SymbolTableStack));
    StackedSymbolTable_PushScope(sstable, "Global" );
}

void StackedSymbolTable_Clear(StackedSymbolTable *sstable)
{
    while(!Stack_IsEmpty(&(sstable->SymbolTableStack)))
    {
        StackedSymbolTable_PopScope(sstable);
    }
    List_Clear(&(sstable->SymbolTableStack));
}

/******************************************************************************
*  PushScope -- This method creates a new symbol table and pushes it onto the
*  symbol table stack.  The top symbol table on the stack represents the
*  innermost symbol scope.
******************************************************************************/
void StackedSymbolTable_PushScope(StackedSymbolTable *sstable, LPCSTR scopeName )
{

    SymbolTable *newSymbolTable = NULL;

    newSymbolTable = (SymbolTable *)malloc(sizeof(SymbolTable));
    //We have to have a name for this scope, so if we got NULL, then use ""
    if (scopeName)
    {
        SymbolTable_Init(newSymbolTable, scopeName);
    }
    else
    {
        SymbolTable_Init(newSymbolTable, "");
    }

    //Add the new symboltable to the stack
    Stack_Push(&(sstable->SymbolTableStack), (void *)newSymbolTable );
}


/******************************************************************************
*  TopScope -- This method retrieves the topmost symbol table from the symbol
*  table stack, and passes it back to the caller without removing it.
******************************************************************************/
SymbolTable *StackedSymbolTable_TopScope(StackedSymbolTable *sstable)
{
    return (SymbolTable *)Stack_Top(&(sstable->SymbolTableStack));
}

/******************************************************************************
*  PopScope -- This method pops the topmost symbol table off the stack,
*  destroying it in the process.  Any symbols in this table have gone out of
*  scope and should not be found in a symbol search.
******************************************************************************/
void StackedSymbolTable_PopScope(StackedSymbolTable *sstable)
{
    SymbolTable *pSymbolTable = NULL;
    pSymbolTable = (SymbolTable *)Stack_Top(&(sstable->SymbolTableStack));
    Stack_Pop(&(sstable->SymbolTableStack));
    if(pSymbolTable)
    {
        SymbolTable_Clear(pSymbolTable);
        free((void *)pSymbolTable);
    }
}

/******************************************************************************
*  FindSymbol -- This method searches the symbol table for the symbol
*  we're looking for.
*  Parameters: symbolName -- LPCSTR which contains the name of the desired
*                            symbol.
*              pp_theSymbol -- CSymbol** which receives the address of the
*                              CSymbol, or NULL if no symbol is found.
*  Returns: true if the symbol is found.
*           false otherwise.
******************************************************************************/
BOOL StackedSymbolTable_FindSymbol(StackedSymbolTable *sstable, LPCSTR symbolName,
                                   Symbol **pp_theSymbol )
{
    SymbolTable *currentSymbolTable = NULL;
    BOOL bFound = FALSE;
    int i, size;
    FOREACH( sstable->SymbolTableStack,
             currentSymbolTable = (SymbolTable *)List_Retrieve(&(sstable->SymbolTableStack));
             bFound = SymbolTable_FindSymbol(currentSymbolTable, symbolName, pp_theSymbol );

             if(bFound) break;
           );

    //Restore the stack so push and pop work correctly.
    List_Reset(&(sstable->SymbolTableStack));
    return bFound;
}

/******************************************************************************
*  AddSymbol -- This method adds a CSymbol* to the topmost symbol table on the
*  stack.
*  Parameters: p_theSymbol -- address of the CSymbol to be added to the symbol
*                             table.
*  Returns:
******************************************************************************/
void StackedSymbolTable_AddSymbol(StackedSymbolTable *sstable, Symbol *p_theSymbol )
{
    SymbolTable *topSymbolTable = NULL;
    topSymbolTable = (SymbolTable *)Stack_Top(&(sstable->SymbolTableStack));
    SymbolTable_AddSymbol(topSymbolTable, p_theSymbol );
}
