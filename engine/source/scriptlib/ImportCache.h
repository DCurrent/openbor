/*
 * OpenBOR - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef IMPORTCACHE_H
#define IMPORTCACHE_H

struct ImportNode;
typedef struct ImportNode ImportNode;

void ImportCache_Init(List *builtinFunctions);
ImportNode *ImportCache_ImportFile(const char *path);
void ImportCache_Clear();
Instruction **ImportList_GetFunctionPointer(List *list, const char *name);

#endif

