/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "SymbolTable.h"
#include <stdio.h>


void Symbol_Init(Symbol* symbol, LPCSTR theName, DWORD flags,
				 ScriptVariant* pvar, Instruction*  theRef)
{
   memset(symbol, 0, sizeof(Symbol));
   if(theName) strcpy(symbol->name, theName);
   else symbol->name[0] = 0;
   symbol->dwFlags = flags;
   ScriptVariant_Init(&(symbol->var));
   if(pvar) ScriptVariant_Copy(&(symbol->var), pvar);
   symbol->theRef = theRef;
}

//------------------------------------------------------------------


void SymbolTable_Init(SymbolTable* stable, LPCSTR theName )
{
   List_Init(&(stable->SymbolList));
   stable->nextSymbolCount = 0;
   if(theName) strcpy(stable->name, theName);
   else stable->name[0] = 0;
}


void SymbolTable_Clear(SymbolTable* stable)
{
   Symbol* psymbol = NULL;
   int i,size;
   FOREACH( stable->SymbolList,
	  psymbol = (Symbol*)List_Retrieve(&(stable->SymbolList));
	  if(psymbol)
	  {
		 ScriptVariant_Clear(&(psymbol->var));
		 free(psymbol);
	  }
   );
   List_Clear(&(stable->SymbolList));
}


/******************************************************************************
*  FindSymbol -- Using the name of the symbol, this method searches the symbol
*  table.
*  Parameters: symbolName -- LPCOLESTR which contains the name of the desired
*                            symbol.
*              pp_theSymbol -- CSymbol** which receives the address of the
*                              CSymbol, or NULL if no symbol is found.
*  Returns: true if the symbol is found.
*           false otherwise.
******************************************************************************/
BOOL SymbolTable_FindSymbol(SymbolTable* stable, LPCSTR symbolName, Symbol** pp_theSymbol )
{
   if (symbolName && List_FindByName(&(stable->SymbolList), symbolName )){
	  *pp_theSymbol = (Symbol*)List_Retrieve(&(stable->SymbolList));
	  return TRUE;
   }
   else
	  return FALSE;
}

/******************************************************************************
*  AddSymbol -- This method adds a CSymbol* to the symbol table.
*  Parameters: p_theSymbol -- address of the CSymbol to be added to the symbol
*                             table.
*  Returns:
******************************************************************************/
void SymbolTable_AddSymbol(SymbolTable* stable, Symbol* p_theSymbol )
{
   List_InsertAfter( &(stable->SymbolList), (void*)p_theSymbol, p_theSymbol->name);
}

