/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2013 OpenBOR Team
 */

/* Author: Plombo
 * Originally started on February 27, 2011; put on hiatus a few weeks later.
 * Restarted January 24, 2013, when all of this file except the readscript()
 * function was rewritten, and #import is fully functional now.
 */

#include <stdio.h>
#include "globals.h"
#include "Interpreter.h"
#include "List.h"
#include "Instruction.h"
#include "packfile.h"
#include "ImportCache.h"

//#define IC_DEBUG 1

struct ImportNode
{
    Interpreter interpreter;
    List functions; // values are Instruction**; names are function names
};

List *builtins; // builtin script functions (drawstring, getlocalvar, etc.)
List imports; // values are ImportNode*; names are lowercased, forward-slashed paths

/**
 * Reads a script file into an allocated buffer.  Be sure to call free() on the
 * returned buffer when you are done with it!
 *
 * Returns the buffer on success, NULL on failure.
 */
char *readscript(const char *path)
{
    int handle = openpackfile(path, packfile);
    int size;
    char *buffer = NULL;

    if(handle < 0)
    {
        goto error;
    }
    size = seekpackfile(handle, 0, SEEK_END);
    if(size < 0)
    {
        goto error;
    }
    buffer = malloc(size + 1);
    if(buffer == NULL)
    {
        goto error;
    }

    if(seekpackfile(handle, 0, SEEK_SET) < 0)
    {
        goto error;
    }
    if(readpackfile(handle, buffer, size) < 0)
    {
        goto error;
    }
    closepackfile(handle);
    buffer[size] = '\0';
    return buffer;

error:
    // ideally, this error message would include the name of the file that tried to import this file
    printf("Script error: unable to open file '%s' for importing\n", path);
    if(buffer)
    {
        free(buffer);
    }
    if(handle >= 0)
    {
        closepackfile(handle);
    }
    return NULL;
}

/**
 * Loads and compiles a script file and indexes its functions so that they can
 * be imported.
 */
HRESULT ImportNode_Init(ImportNode *self, const char *path)
{
    char *scriptText;
    int i, size;
    List *list; // more readable than "&self->interpreter.theInstructionList"

    List_Init(&self->functions);
    Interpreter_Init(&self->interpreter, path, builtins);
    self->interpreter.theParser.isImport = TRUE;
    scriptText = readscript(path);
    if(scriptText == NULL)
    {
        goto error;
    }
    if(FAILED(Interpreter_ParseText(&self->interpreter, scriptText, 1, path)))
    {
        printf("Script error: failed to import '%s': parsing failed\n", path);
        goto error;
    }
    free(scriptText);
    scriptText = NULL;

    // get indices of the function declarations in the instruction list
    list = &self->interpreter.theInstructionList;
    size = List_GetSize(list);
    List_Reset(list);
    for(i = 0; i < size; i++)
    {
        Instruction *inst = (Instruction *)List_Retrieve(list);
        if(inst->OpCode == FUNCDECL)
        {
#ifdef IC_DEBUG
            fprintf(stderr, "ImportNode_Init: %s: %s@%i\n", path, List_GetName(list), i);
#endif
            List_InsertAfter(&self->functions, (void *)(size_t)i, List_GetName(list));
        }
        List_GotoNext(list);
    }

    // finish compiling and convert indices to pointers to the function entry points
    if(FAILED(Interpreter_CompileInstructions(&self->interpreter)))
    {
        printf("Script error: failed to import '%s': failed to compile\n", path);
        goto error;
    }
    assert(list->solidlist != NULL);
    List_Reset(&self->functions);
    size = List_GetSize(&self->functions);
    for(i = 0; i < size; i++)
    {
        int index = (size_t)List_Retrieve(&self->functions);
        List_Update(&self->functions, &(list->solidlist[index]));
        assert(((Instruction *)(list->solidlist[index]))->OpCode == FUNCDECL);
        List_GotoNext(&self->functions);
    }

    return S_OK;

error:
    if(scriptText)
    {
        free(scriptText);
    }
    Interpreter_Clear(&self->interpreter);
    List_Clear(&self->functions);
    return E_FAIL;
}

/**
 * Returns a pointer to the function entry point if this script has a function
 * with the specified name; returns NULL otherwise.
 */
Instruction **ImportNode_GetFunctionPointer(ImportNode *self, const char *name)
{
    if(List_FindByName(&self->functions, name))
    {
        return (Instruction **)List_Retrieve(&self->functions);
    }
    else
    {
        return NULL;
    }
}

void ImportNode_Clear(ImportNode *self)
{
    Interpreter_Clear(&self->interpreter);
    List_Clear(&self->functions);
}

/**
 * From a list of ImportNodes, finds and returns a pointer to the function with
 * the specified name in the list. Script files at the *end* of the import list
 * have priority, so if the modder imports two script files with the same
 * function name, the function used will be from the file imported last.
 */
Instruction **ImportList_GetFunctionPointer(List *list, const char *name)
{
    int i;
    List_GotoLast(list);
    for(i = List_GetSize(list); i > 0; i--)
    {
        ImportNode *node = List_Retrieve(list);
        Instruction **inst = ImportNode_GetFunctionPointer(node, name);
        if(inst != NULL)
        {
#ifdef IC_DEBUG
            fprintf(stderr, "importing '%s' from '%s'\n", name, List_GetName(list));
#endif
            return inst;
        }
        List_GotoPrevious(list);
    }
    return NULL;
}

/**
 * Initializes the import cache.
 */
void ImportCache_Init(List *builtinFunctions)
{
    builtins = builtinFunctions;
    List_Init(&imports);
}

/**
 * Returns a pointer to the ImportNode for the specified file.
 *
 * If the file has already been imported, this function returns the pointer to
 * the existing node.  If it hasn't, it imports it and then returns the pointer.
 *
 * Returns NULL if the file hasn't already been imported and an error occurs
 * when compiling it.
 */
ImportNode *ImportCache_ImportFile(const char *path)
{
    int i;
    char path2[256];
    ImportNode *node;

    // first convert the path to standard form, lowercase with forward slashes
    assert(strlen(path) <= 255);
    for(i = strlen(path); i >= 0; i--)
    {
        if(path[i] == '\\')
        {
            path2[i] = '/';
        }
        else if(path[i] >= 'A' && path[i] <= 'Z')
        {
            path2[i] = path[i] + ('a' - 'A');
        }
        else
        {
            path2[i] = path[i];
        }
    }
#ifdef IC_DEBUG
    fprintf(stderr, "ImportCache_ImportFile: '%s' -> '%s'\n", path, path2);
#endif

    // find and return node if this file has already been imported
    if(List_FindByName(&imports, path2))
    {
        return (ImportNode *)List_Retrieve(&imports);
    }

    // otherwise, create a new node for this file, add it to the cache, and return it
    node = malloc(sizeof(ImportNode));
    if(FAILED(ImportNode_Init(node, path2)))
    {
        free(node);
        return NULL;
    }
    List_GotoLast(&imports);
    List_InsertAfter(&imports, node, path2);
    return node;
}

/**
 * Frees all of the imported scripts. Called when shutting down the script
 * engine.
 */
void ImportCache_Clear()
{
    List_Reset(&imports);
    while(List_GetSize(&imports))
    {
        ImportNode *node = (ImportNode *)List_Retrieve(&imports);
        ImportNode_Clear(node);
        free(node);
        List_Remove(&imports);
    }
    List_Clear(&imports);
}


