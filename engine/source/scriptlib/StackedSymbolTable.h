/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef STACKEDSYMBOLTABLE_H
#define STACKEDSYMBOLTABLE_H

#include "SymbolTable.h"
#include "Stack.h"
typedef struct StackedSymbolTable
{
    Stack SymbolTableStack;
    CHAR name[MAX_STR_LEN + 1];
} StackedSymbolTable;

void StackedSymbolTable_Init(StackedSymbolTable *sstable, LPCSTR theName );
void StackedSymbolTable_Clear(StackedSymbolTable *sstable);
void StackedSymbolTable_PushScope(StackedSymbolTable *sstable, LPCSTR scopeName ) ;
SymbolTable *StackedSymbolTable_TopScope(StackedSymbolTable *sstable);
void StackedSymbolTable_PopScope(StackedSymbolTable *sstable) ;
BOOL StackedSymbolTable_FindSymbol(StackedSymbolTable *sstable, LPCSTR symbolName,
                                   Symbol **pp_theSymbol );
void StackedSymbolTable_AddSymbol(StackedSymbolTable *sstable, Symbol *p_theSymbol ) ;
#endif

