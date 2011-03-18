/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// Author: Plombo
// Start date: Feb 27 2011

#ifndef IMPORTCACHE_H
#define IMPORTCACHE_H

#include "depends.h"
#include "Interpreter.h"

typedef struct ImportNode {
	int numRefs;
	Interpreter interpreter;
} ImportNode;

HRESULT ImportNode_Init(ImportNode* node, const char* path);
void ImportNode_Clear(ImportNode* node);

void ImportCache_Init();
ImportNode* ImportCache_Retrieve(const char* path);
void ImportCache_Release(ImportNode* node);
void ImportCache_Clear();

#endif

