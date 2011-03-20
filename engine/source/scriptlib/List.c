/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "List.h"
#include <assert.h>

#ifdef DEBUG
void chklist(List* list) {
	assert(list);
	assert(list->initdone == 1); // method called on uninitialised list
}
#endif

#ifdef USE_STRING_HASHES
unsigned char strhash(char* s) {	
	ptrdiff_t tmp = 0;
	char* p = s;
	while(*p) {
		tmp += *p - 'A';
		p++;
	}
	return (size_t)tmp % 256;
}

/* add a single node to the hash list */
void List_AddHash(List* list, Node* node) {
	#ifdef DEBUG
	chklist((List*)list);
	#endif
	unsigned char h;
	size_t save;
	
	assert(node);
	if(!node->name) return;
	
	if (!list->buckets) list->buckets = calloc(1, sizeof(Bucket*) * 256);
	
	h = strhash((char*)node->name);
	if (!list->buckets[h]) {
		list->buckets[h] = calloc(1, sizeof(Bucket));
		list->buckets[h]->nodes = calloc(1, sizeof(Node*) * 8);
		assert(list->buckets[h]->nodes != NULL);
		list->buckets[h]->size = 8;
	}
	
	save = list->buckets[h]->size;
	assert(list->buckets[h]->used <= save);
	if (list->buckets[h]->used == save) {
		list->buckets[h]->nodes = realloc(list->buckets[h]->nodes, sizeof(Node*) * (save * 2));
		assert(list->buckets[h]->nodes != NULL);
		list->buckets[h]->size = save * 2;
	}
	
	list->buckets[h]->nodes[list->buckets[h]->used] = node;
	list->buckets[h]->used++;
}

/* removes the last element from the index list
only use on fully indexed list */
void List_RemoveHash(List* list, Node* node) {
	#ifdef DEBUG
	chklist((List*)list);
	#endif
	unsigned char h;
	size_t i;
	if(!node->name) return;
	h = strhash((char*)node->name);
	assert(list->buckets[h]);
	assert(list->buckets[h]->used > 0);
	for(i=0;i<list->buckets[h]->used;i++) {
		if (node ==  list->buckets[h]->nodes[i]) {
			list->buckets[h]->nodes[i] = NULL;
			break;
		}
	}
}

/* free everything related to the string hash list 
usually you dont have to do it manually, since its called by List_Clear
but it won't hurt either */
void List_FreeHashes(List* list) {
	#ifdef DEBUG
	chklist((List*)list);
	#endif
	int i;
	if(!list->buckets) return;
	for (i=0;i<256;i++) {
		if(list->buckets[i]) {
			free(list->buckets[i]->nodes);
			free(list->buckets[i]);
		}		
	}
	free(list->buckets);
	list->buckets = NULL;
}

/* build hashes for entire list
this is only required when doing a list copy.
*/
void List_CreateHashes(List* list) {
	#ifdef DEBUG
	chklist((List*)list);
	#endif
	Node* n = list->first;
	while(n) {
		List_AddHash(list, n);
		n = n->next;
	}		
}
#endif

#ifdef USE_INDEX
unsigned char ptrhash(void* value) {
	size_t tmp = (size_t) value;
	tmp >>= 4;
	return tmp % 256;
}

/* add a single node to the index list */
void List_AddIndex(List* list, Node* node, size_t index) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	unsigned char h;
	size_t save;
	
	assert(node);	
	
	if (!list->mindices) 
		list->mindices = calloc(1, sizeof(LIndex*) * 256);
	
	h = ptrhash(node->value);
	if (!list->mindices[h]) {
		list->mindices[h] = calloc(1, sizeof(LIndex));
		list->mindices[h]->nodes = calloc(1, sizeof(Node*) * 8);
		assert(list->mindices[h]->nodes != NULL);
		list->mindices[h]->indices = calloc(1, sizeof(ptrdiff_t) * 8);
		assert(list->mindices[h]->indices != NULL);		
		list->mindices[h]->size = 8;
	}
	
	save = list->mindices[h]->size;
	assert(list->mindices[h]->used <= save);
	if (list->mindices[h]->used == save) {
		list->mindices[h]->nodes = realloc(list->mindices[h]->nodes, sizeof(Node*) * (save * 2));
		assert(list->mindices[h]->nodes != NULL);
		list->mindices[h]->indices = realloc(list->mindices[h]->indices, sizeof(ptrdiff_t) * (save * 2));
		assert(list->mindices[h]->indices != NULL);		
		list->mindices[h]->size = save * 2;
	}
	
	list->mindices[h]->nodes[list->mindices[h]->used] = node;
	list->mindices[h]->indices[list->mindices[h]->used] = index;
	list->mindices[h]->used++;
}

/* removes the last element from the index list
   only use on fully indexed list */
void List_RemoveLastIndex(List* list) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	unsigned char h;
	assert(list->last);
	assert(list->current);
	assert(list->current == list->last);
	h = ptrhash(list->last->value);
	assert(list->mindices[h]);
	assert(list->mindices[h]->used > 0);
	list->mindices[h]->used--; // it would be wrong to do this to a random element, but it's ok for the last one, since it was added as last
	list->mindices[h]->nodes[list->mindices[h]->used] = NULL;
	list->mindices[h]->indices[list->mindices[h]->used] = 0;
}

/* build indices for entire list
   the indices will be destroyed whenever an element is either 
   inserted or removed from the list, 
   except if it was the last node/inserted after the last node */
void List_CreateIndices(List* list) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	Node* n = list->first;
	size_t index = 0;
	while(n) {
		List_AddIndex(list, n, index);
		index++;
		n = n->next;
	}		
}

/* free everything related to the index list 
   usually you dont have to do it manually, since its called by List_Clear
   but it won't hurt either */
void List_FreeIndices(List* list) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	int i;
	if(!list->mindices) return;
	for (i=0;i<256;i++) {
		if(list->mindices[i]) {
			free(list->mindices[i]->indices);
			free(list->mindices[i]->nodes);
			free(list->mindices[i]);
		}		
	}
	free(list->mindices);
	list->mindices = NULL;
}
#endif

void List_GotoLast(List* list)
{
#ifdef DEBUG
	chklist((List*)list);
#endif
#ifdef LIST_DEBUG
	printf("List_Last %p\n", list);
#endif
	list->current = list->last;
}

void List_GotoFirst(List* list) {
#ifdef DEBUG
	chklist((List*)list);
#endif
#ifdef LIST_DEBUG
	printf("List_First %p\n", list);
#endif
	list->current = list->first;
}

Node* List_GetCurrent(List* list) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	return list == NULL ? NULL : list->current;
}

void List_SetCurrent(List* list, Node* current) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	if (list) list->current = current;
}


void Node_Clear(Node* node)
{
	if(!node) return;
	if(node->name) free((void*)node->name);
}

void List_Init(List* list)
{
#ifdef LIST_DEBUG
	printf("List_Init %p\n", list);
#endif
	list->first = list->current = list->last = NULL;
	list->size = list->index = 0;
	list->solidlist = NULL;
#ifdef USE_INDEX
	list->mindices = NULL;	
#endif
#ifdef USE_STRING_HASHES
	list->buckets = NULL;
#endif
#ifdef DEBUG	
	list->initdone = 1;
#endif
}

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


int List_GetNodeIndex(List* list, Node* node) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	int i;
	Node* n;
	#ifdef USE_INDEX
	unsigned char h;	
	if(list->mindices) {
		h = ptrhash(node->value);
		assert(list->mindices[h]);
		for(i=0; i<list->mindices[h]->used; i++) {
			//assert(list->mindices[h]->nodes[i]); gets overwritten by update with NULL
			if(list->mindices[h]->nodes[i] && list->mindices[h]->nodes[i] == node) 
				return list->mindices[h]->indices[i];
		}
		return -1;
	} else	
	#endif
	{
		n = list->first;
		i = 0;
		while(n) {
			if(n == node) return i;
			i++;
			n = n->next;
		}
		return -1;
	}
}

int List_GetIndex(List* list)
{
#ifdef DEBUG
	chklist((List*)list);
#endif
	return List_GetNodeIndex(list, list->current);
}

void List_Copy(List* listdest, const List* listsrc)
{
#ifdef DEBUG
	chklist((List*)listsrc);
#endif
#ifdef LIST_DEBUG
	printf("List_Copy %p %p\n", listsrc, listdest);
#endif
	Node *lptr = listsrc->first;
	Node *nptr;
	int i = 0, curr = -1;
	
	List_Init(listdest);
	if (lptr == NULL) return;
	//create the first Node
	nptr = (Node*)malloc(sizeof(Node));
	nptr->value = lptr->value;
	nptr->name = NAME(lptr->name);
	nptr->prev = NULL;
	nptr->next = NULL;
	
	listdest->current = nptr;
	listdest->first = nptr;
	listdest->last = nptr;
	listdest->size = 1;
	
	if (listsrc->current == lptr)
		curr = i;
	while ((lptr = lptr->next) != NULL) {
		i++;
		if (listsrc->current == lptr)
			curr = i;
		List_InsertAfter(listdest, lptr->value, lptr->name);
		
	}
	assert(curr != -1);
	List_GotoFirst(listdest);
	for(i=0;i<curr;i++)
		List_GotoNext(listdest); //setting current to the right value
		
	#ifdef USE_INDEX
	if(listsrc->mindices)
		List_CreateIndices(listdest);
	#endif
	
	#ifdef USE_STRING_HASHES
	List_CreateHashes(listdest);
	#endif
	
}

void List_Clear(List* list)
{
#ifdef DEBUG
	chklist((List*)list);
#endif
#ifdef LIST_DEBUG
	printf("List_clear %p \n", list);
#endif
	
	//Delete all the Nodes in the list.
	Node* nptr = list->first;
	list->current = list->first;
	
	while(list->current)
	{
		list->current = list->current->next;
		Node_Clear(nptr);
		free(nptr);
		nptr = list->current;
	}
	if(list->solidlist)
	{
		free(list->solidlist);
		list->solidlist = NULL;
	}

	#ifdef USE_INDEX
	if(list->mindices)
		List_FreeIndices(list);
	#endif
	
	#ifdef USE_STRING_HASHES
	List_FreeHashes(list);
	#endif
	
	List_Init(list);
}

//Insertion functions
void List_InsertBefore(List* list, void* e, char* theName)
{
#ifdef DEBUG
	chklist((List*)list);
#endif
#ifdef LIST_DEBUG
	printf("List_InsertBefore %p %s\n", list, theName ? theName : "no-name");
#endif
	#ifdef USE_INDEX
	if (list->mindices) 
		List_FreeIndices(list); // inserting something before something else destroys our indices list.
	#endif
	
	//Construct a new Node
	Node *nptr = (Node*)malloc(sizeof(Node));
	assert(nptr != NULL);
	
	nptr->value = e;
	nptr->name = NAME(theName);
	
	#ifdef USE_STRING_HASHES
	List_AddHash(list, nptr);
	#endif
	
	if (list->size == 0)
	{
		nptr->next = NULL;
		nptr->prev = NULL;
		list->current = list->first = list->last = nptr;
	}
	else
	{
		nptr->next = list->current;
		nptr->prev = list->current->prev;
		if (list->current->prev != NULL)
			list->current->prev->next = nptr;
		list->current->prev = nptr;
		if (list->current == list->first) list->first = nptr;
		list->current = nptr;
	}
	list->size++;
}

void List_InsertAfter(List* list, void* e, char* theName) {
#ifdef DEBUG
	chklist((List*)list);
#endif
#ifdef LIST_DEBUG
	printf("List_InsertAfter %p %s\n", list, theName ? theName : "no-name");
#endif
	#ifdef USE_INDEX
	int doIndex = 0;
	if (list->mindices) {
		if(list->current != list->last) 
			List_FreeIndices(list); // inserting something in the middle of something else destroys our indices list.
		else
			doIndex = 1;
	}
	#endif
	
	//Construct a new Node and fill it with the appropriate value
	Node *nptr = (Node*)malloc(sizeof(Node));
	
	assert(nptr != NULL);
	nptr->value = e;
	nptr->name = NAME(theName);
	
	#ifdef USE_STRING_HASHES
	List_AddHash(list, nptr);
	#endif	
	
	if (list->size == 0)
	{
		nptr->prev = NULL;
		nptr->next = NULL;
		list->current = list->first = list->last = nptr;
	}
	else
	{
		nptr->next = list->current->next;
		nptr->prev = list->current;
		if (list->current->next != NULL) list->current->next->prev = nptr;
		list->current->next = nptr;
		if (list->current == list->last)
			list->last = nptr;
		list->current = nptr;
	}	
	#ifdef USE_INDEX
	if (doIndex) 
		List_AddIndex(list, list->current, list->size);
	#endif
	list->size++;
}

//removes the current node and sets current to next, if applicable
void List_Remove(List* list) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	Node *nptr;
#ifdef LIST_DEBUG
	printf("List_Remove %p\n", list);
#endif
	if (list->size == 0)
	{
		//OutputDebugString("Attempted to remove from empty list.\n");
		return;
	}
	else if (list->size == 1)
	{
		#ifdef USE_STRING_HASHES
		List_RemoveHash(list, list->current);
		#endif		
		Node_Clear(list->current);
		free(list->current);
		list->first = list->current = list->last = NULL;
		#ifdef USE_INDEX
		List_FreeIndices(list);
		#endif
	}
	else
	{
		#ifdef USE_INDEX
		if(list->mindices) {
			if (list->current != list->last)
				List_FreeIndices(list); // removing something before something else destroys our indices list.
			else
				List_RemoveLastIndex(list);
		}
		#endif
		
		if(list->current->prev != NULL)
			list->current->prev->next = list->current->next;
		if(list->current->next)
			list->current->next->prev = list->current->prev;
		if (list->current == list->last)
			nptr = list->current->prev;
		else
			nptr = list->current->next;
		
		#ifdef USE_STRING_HASHES
		List_RemoveHash(list, list->current);
		#endif
		
		Node_Clear(list->current);
		free(list->current);
		
		if (list->current == list->last)
			list->last = nptr;
		if (list->current == list->first)
			list->first = nptr;
		list->current = nptr;
	}
	list->size--;
}

void List_GotoNext(List* list)
{
#ifdef DEBUG
	chklist((List*)list);
#endif
#ifdef LIST_DEBUG
	printf("List_Next %p\n", list);
#endif
	if (list->current != list->last)
		list->current = list->current->next;
}

void List_GotoPrevious(List* list)
{
#ifdef DEBUG
	chklist((List*)list);
#endif
#ifdef LIST_DEBUG
	printf("List_Prev %p\n", list);
#endif
	if (list->current->prev)
		list->current = list->current->prev;
}

void* List_Retrieve(const List* list)
{
#ifdef DEBUG
	chklist((List*)list);
#endif
	if (list->current)
		return list->current->value;
	else
		return NULL;
}

void* List_GetFirst(const List* list)
{
#ifdef DEBUG
	chklist((List*)list);
#endif
	if (list->first)
		return list->first->value;
	else
		return NULL;
}

void* List_GetLast(const List* list)
{
#ifdef DEBUG
	chklist((List*)list);
#endif
	if (list->last)
		return list->last->value;
	else
		return NULL;
}

void List_Update(List* list, void* e) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	#ifdef USE_INDEX
	unsigned char h;
	ptrdiff_t save, i;
	if(list->mindices) {
		h = ptrhash(list->current->value);
		assert(list->mindices[h]);
		for(i=0;i<list->mindices[h]->used;i++) {
			if(list->mindices[h]->nodes[i] == list->current) {
				list->mindices[h]->nodes[i] = NULL;
				save = list->mindices[h]->indices[i];
				list->mindices[h]->indices[i] = -1;
				list->current->value = e;
				List_AddIndex(list, list->current, save);
				break;
			}
		}
	} else
	#endif
	if (list->size != 0)
		list->current->value = e;
}

/* returns the node that contains e, or NULL, if not found */
Node* List_Contains(List* list, void* e) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	Node *n;
#ifdef USE_INDEX
	unsigned char h;
	ptrdiff_t i;
	if(list->mindices) {
		h = ptrhash(e);
		if (!list->mindices[h]) 
			return NULL;
		for(i=0; i<list->mindices[h]->used; i++) {
			//assert(list->indices[h]->nodes[i]); gets overwritten by update with NULL
			if(list->mindices[h]->nodes[i] && list->mindices[h]->nodes[i]->value == e) {
				//gotcha
				return list->mindices[h]->nodes[i];
			}
		}
		return NULL;
	} else
#endif
	{
		n = list->first;
		while (n && (n->value != e))
			n = n->next;
		return n;
	}	
}


/* SIDE EFFECT: Moves list->current to found entity.
use List_Contains if you dont like that.
*/
int List_Includes(List* list, void* e) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	Node* n = List_Contains(list, e);
	if (n) {
		list->current = n;
		return 1;
	}	
	return 0;
}

/* returns the first node of which name is equal to theName */
Node* List_SearchName(List* list, char* theName) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	Node *nptr;
	if (theName == NULL) return NULL;
	
#ifdef USE_STRING_HASHES
	size_t i;
	unsigned char h = strhash((char*)theName);
	if (!list->buckets || !list->buckets[h]) return 0;
	for(i=0;i<list->buckets[h]->used;i++) {
		nptr = list->buckets[h]->nodes[i];
		if(nptr && strcmp(theName, nptr->name)==0) {
			return list->buckets[h]->nodes[i];
		}
	}	
	return NULL;
#else
	nptr = list->first;
	
	while (nptr){
		if (nptr->name){
			if (strcmp( nptr->name, theName ) == 0)
				return nptr;
		}
		nptr = nptr->next;
	}
	return NULL;
#endif
	
}
/* SIDE EFFECTS: sets list->current to the first found node */
int List_FindByName(List* list, char* name) {
	#ifdef DEBUG
	chklist((List*)list);
	#endif
	
	Node* n = List_SearchName(list, name);
	if (n)  {
		list->current = n;
		return 1;
	}
	return 0;
}

char* List_GetName(const List* list) {
#ifdef DEBUG
	chklist((List*)list);
#endif
	if (list->size != 0)
		return list->current->name;
	else
		return NULL;
}

void List_Reset(List* list) {
#ifdef DEBUG
	chklist(list);
#endif
#ifdef LIST_DEBUG
	printf("List_Reset %p\n", list);	
#endif
	list->current = list->first;
}

int List_GetSize(const List* list) {
#ifdef DEBUG	
	chklist((List*)list);
#endif
	return list->size;
}


