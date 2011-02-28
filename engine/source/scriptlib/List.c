/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "List.h"
#include "tracemalloc.h"
#include <assert.h>

void List_GotoLast(List* list)
{
	#ifdef LIST_DEBUG
	printf("List_Last %p\n", list);
	#endif
	list->current = list->last;
}

void List_GotoFirst(List* list) {
	#ifdef LIST_DEBUG
	printf("List_First %p\n", list);
	#endif
	list->current = list->first;
}

Node* List_GetCurrent(List* list) {
	return list == NULL ? NULL : list->current;
}

void List_SetCurrent(List* list, Node* current) {
	if (list) list->current = current;
}


void Node_Clear(Node* node)
{
	if(!node) return;
	if(node->name) tracefree((void*)node->name);
}

void List_Init(List* list)
{
	#ifdef LIST_DEBUG
	printf("List_Init %p\n", list);
	#endif
	list->first = list->current = list->last = NULL;
	list->size = list->index = 0;
	list->solidlist = NULL;
}

void List_Solidify(List* list)
{
	int i = 0;
	#ifdef LIST_DEBUG
	printf("List_Solid %p\n", list);
	#endif
	if(list->solidlist) tracefree(list->solidlist);
	if(!list->size) return;
	List_GotoFirst(list);
	list->solidlist = (void**)tracemalloc("List_Solidify", sizeof(void*)*(list->size));
	while(list->current) {
		list->solidlist[i++] = list->current->value;
		List_Remove(list);
	}
	list->first = list->current = list->last = NULL;
	list->index = 0;
}


int List_GetIndex(List* list)
{
	int i;
	Node *nptr = list->first;
	//if(!list->size) return -1;
	for(i=0; i<list->size; i++)
	{
		if(nptr == list->current) return i;
		nptr = nptr->next;
	}
	return -1;
}

void List_Copy(List* listdest, const List* listsrc)
{
	#ifdef LIST_DEBUG
	printf("List_Copy %p %p\n", listsrc, listdest);
	#endif
	Node *lptr = listsrc->first;
	Node *nptr;
	int i = 0, curr = -1;
	
	//is the list we're copying empty
	if (lptr == NULL)
	{
		listdest->current = listdest->first = listdest->last = NULL;
		listdest->size = 0;
	}
	else
	{
		List_Init(listdest);
		//create the first Node
		nptr = (Node*)tracemalloc("List Copy", sizeof(Node));
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
			
	}
}

void List_Clear(List* list)
{
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
		tracefree(nptr);
		nptr = list->current;
	}
	if(list->solidlist)
	{
		tracefree(list->solidlist);
		list->solidlist = NULL;
	}
	list->first = list->current = list->last = NULL;
	list->size = list->index = 0;
}

//Insertion functions
void List_InsertBefore(List* list, void* e, LPCSTR theName)
{
	#ifdef LIST_DEBUG
	printf("List_InsertBefore %p %s\n", list, theName ? theName : "no-name");
	#endif
	//Construct a new Node
	Node *nptr = (Node*)tracemalloc("List_InsertBefore", sizeof(Node));
	assert(nptr != NULL);
	
	nptr->value = e;
	nptr->name = NAME(theName);
	
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

void List_InsertAfter(List* list, void* e, LPCSTR theName) {
	#ifdef LIST_DEBUG
	printf("List_InsertAfter %p %s\n", list, theName ? theName : "no-name");
	#endif
	
	//Construct a new Node and fill it with the appropriate value
	Node *nptr = (Node*)tracemalloc("List_InsertAfter", sizeof(Node));
	
	assert(nptr != NULL);
	nptr->value = e;
	nptr->name = NAME(theName);
	
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
	list->size++;
}

void List_Remove(List* list)
{
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
		Node_Clear(list->current);
		tracefree(list->current);
		list->first = list->current = list->last = NULL;
	}
	else
	{
		if(list->current->prev != NULL)
			list->current->prev->next = list->current->next;
		if(list->current->next)
			list->current->next->prev = list->current->prev;
		if (list->current == list->last)
			nptr = list->current->prev;
		else
			nptr = list->current->next;
		
		Node_Clear(list->current);
		tracefree(list->current);
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
	#ifdef LIST_DEBUG
	printf("List_Next %p\n", list);
	#endif
	if (list->current != list->last)
		list->current = list->current->next;
}

void List_GotoPrevious(List* list)
{
	#ifdef LIST_DEBUG
	printf("List_Prev %p\n", list);
	#endif
	if (list->current->prev)
		list->current = list->current->prev;
}

void* List_Retrieve(const List* list)
{
	if (list->current)
		return list->current->value;
	else
		return NULL;
}

void* List_GetFirst(const List* list)
{
	if (list->first)
		return list->first->value;
	else
		return NULL;
}

void* List_GetLast(const List* list)
{
	if (list->last)
		return list->last->value;
	else
		return NULL;
}

void List_Update(List* list, void* e)
{
	if (list->size != 0)
		list->current->value = e;
}

int List_Includes(List* list, void* e)
{
	Node *nptr = list->first;
	while (nptr && (nptr->value != e))
		nptr = nptr->next;
	if (nptr)
		list->current = nptr;
	return (nptr != NULL);
}

int List_FindByName(List* list, LPCSTR theName )
{
	Node *nptr = list->first;
	
	while (nptr){
		if (nptr->name){
			if (strcmp( nptr->name, theName ) == 0)
				break;
		}
		nptr = nptr->next;
	}
	
	if (nptr != NULL){
		list->current = nptr;
		return 1;
	}
	else
		return 0;
	
}

LPCSTR List_GetName(const List* list)
{
	if (list->size != 0)
		return list->current->name;
	else
		return NULL;
}

void List_Reset(List* list)
{
	#ifdef LIST_DEBUG
	printf("List_Reset %p\n", list);
	#endif
	list->current = list->first;
}

int List_GetSize(const List* list)
{
	return list->size;
}


