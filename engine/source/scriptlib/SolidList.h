/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef _SOLIDLIST_H_
#define _SOLIDLIST_H_

#include <stddef.h>
#include "List.h"

typedef struct {
	size_t size;
	void **solidlist;
} SolidList;

// makes a solidlist from a list
// side effects: List_Clear will be called on the List.
SolidList* SolidListFromList(List* list);

void freeSolidList(SolidList* s);

#endif
