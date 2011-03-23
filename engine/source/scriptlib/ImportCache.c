/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// Author: Plombo
// Start date: Feb 27 2011

#include <stdio.h>
#include "depends.h"
#include "ImportCache.h"
#include "List.h"
#include "Interpreter.h"
#include "packfile.h"
#include "globals.h"
#include "openborscript.h" // for extern declaration of "theFunctionList"

// global list of loaded importable scripts
List importCache;

// returns buffer on success, NULL on failure
char* readscript(const char* path)
{
	int handle = openpackfile(path, packfile);
	int size;
	char* buffer = NULL;

	if(handle < 0) goto error;
	size = seekpackfile(handle, 0, SEEK_END);
	if(size < 0) goto error;
	buffer = malloc(size + 1);
	if(buffer == NULL) goto error;

	if(seekpackfile(handle, 0, SEEK_SET) < 0) goto error;
	if(readpackfile(handle, buffer, size) < 0) goto error;
	closepackfile(handle);
	buffer[size] = '\0';
	return buffer;

error:
	// FIXME: this error message should include the name of the file that tried to import this file
	if(buffer) free(buffer);
	if(handle >= 0) closepackfile(handle);
	return NULL;
}

HRESULT ImportNode_Init(ImportNode* node, const char* path)
{
	char* scriptText;
	node->numRefs = 0;

	Interpreter_Init(&node->interpreter, "#import", &theFunctionList);
	scriptText = readscript(path);
	if(scriptText == NULL) goto error;
	if(FAILED(Interpreter_ParseText(&node->interpreter, scriptText, 1, path))) goto error;
	if(FAILED(Interpreter_CompileInstructions(&node->interpreter))) goto error;
	free(scriptText);
	return S_OK;

error:
	printf("Script error: unable to import script file '%s'\n");
	return E_FAIL;
}

void ImportNode_Clear(ImportNode* node)
{
	Interpreter_Clear(&node->interpreter);
}

void ImportCache_Init()
{
	List_Init(&importCache);
}

// returns pointer to node on success, NULL on failure
ImportNode* ImportCache_Retrieve(const char* path)
{
	ImportNode* node;

	if(List_FindByName(&importCache, (char*) path)) // already imported by another file
	{
#ifdef VERBOSE
		printf("Reusing import %s\n", path);
#endif
		node = List_Retrieve(&importCache);
		node->numRefs++;
	}
	else // file is being imported for the first time
	{
#ifdef VERBOSE
		printf("Allocating import %s\n", path);
#endif
		node = malloc(sizeof(ImportNode));
		if(FAILED(ImportNode_Init(node, path))) { free(node); return NULL; }
		node->numRefs = 0;
		List_GotoLast(&importCache);
		List_InsertAfter(&importCache, node, (char*) path);
		node->numRefs++;
	}

	return node;
}

// releases a reference to an ImportNode; must be called for ImportNode to be freed properly!
void ImportCache_Release(ImportNode* node)
{
	node->numRefs--;
	if(node->numRefs <= 0)
	{
		ImportNode_Clear(node);
		assert(List_Includes(&importCache, node));
		List_Remove(&importCache);
		free(node);
	}
}

void ImportCache_Clear()
{
	int i, num;
	ImportNode* node;

	List_Reset(&importCache);
	while(importCache.size > 0)
	{
		node = List_Retrieve(&importCache);
		printf("Warning: imported script '%s' not freed (%d unreleased references)\n", List_GetName(&importCache), node->numRefs);
		num = node->numRefs;
		for(i=0; i<num; i++)
			ImportCache_Release(node);
		assert(!List_GetNodeByValue(&importCache, node));
	}
	List_Clear(&importCache);
}

