/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <string.h>
#include "SolidList.h"

SolidList* SolidListFromList(List* list) {
	SolidList* result;
	assert(list);
	size_t sldsize = sizeof(void*)*(list->size);
	size_t i = 0;
	if(!list->size) return NULL;
	result = malloc(sizeof(SolidList));
	if(!result) return NULL;
	result->solidlist = (void**)malloc(sldsize);
	result->size = list->size;
	List_GotoFirst(list);
	while(list->current) {
		result->solidlist[i++] = list->current->value;
		List_Remove(list);
	}
	List_Clear(list);
	return result;
}

void freeSolidList(SolidList* s) {
	if(!s) return;
	if(s->solidlist)
		free(s->solidlist);
	free(s);
}

/*
void List_Solidify(List* list)
{
#ifdef DEBUG
	chklist((List*)list);
#endif
	int i = 0;
	size_t sldsize = 0;
	size_t savesize = 0;

#ifdef LIST_DEBUG
	printf("List_Solidify %p\n", list);
#endif
	if(list->solidlist) free(list->solidlist);
	if(!list->size) return;

	sldsize = sizeof(void*)*(list->size);
	list->solidlist = (void**)malloc(sldsize);

	savesize = list->size;

	List_GotoFirst(list);
	while(list->current) {
		list->solidlist[i++] = list->current->value;
		List_Remove(list);
	}

	list->size = savesize; // the size is accessed by some piece of code after solidify, so restore it.

#ifdef LIST_DEBUG
	printf("solidlist of %p:\n", list);
#endif
	list->first = list->current = list->last = NULL;
	list->index = 0;

	//this shouldnt be needed when using remove, but it dont hurt either
	#ifdef USE_INDEX
	if(list->mindices)
		List_FreeIndices(list);
	#endif
	#ifdef USE_STRING_HASHES
	List_FreeHashes(list);
	#endif
}
*/
