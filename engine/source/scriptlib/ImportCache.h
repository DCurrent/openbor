/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2013 OpenBOR Team
 */

#ifndef IMPORTCACHE_H
#define IMPORTCACHE_H

struct ImportNode;
typedef struct ImportNode ImportNode;

void ImportCache_Init();
ImportNode *ImportCache_ImportFile(const char *path);
void ImportCache_Clear();
Instruction **ImportList_GetFunctionPointer(List *list, const char *name);

#endif

