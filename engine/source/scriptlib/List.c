/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include "List.h"
#include "tracemalloc.h"

void Node_Clear(Node* node)
{
	if(!node) return;
	if(node->name) tracefree((void*)node->name);
	//memset(node, 0, sizeof(Node));
}

void List_Init(List* list)
{
	list->first = list->current = list->last = NULL;
	list->size = list->index = 0;
	list->solidlist = NULL;
}

void List_Solidify(List* list)
{
	int i;
	Node *tp;
	Node *nptr = list->first;
	if(list->solidlist) tracefree(list->solidlist);
	if(!list->size) return;
	list->solidlist = (void**)tracemalloc("List_Solidify", sizeof(void*)*(list->size));
	for(i=0; i<list->size; i++)
	{
		list->solidlist[i] = nptr->value;
		if(nptr->name) tracefree((void*)nptr->name);
		tp = nptr;
		nptr = nptr->next;
		tracefree(tp);
		tp = NULL;
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
	Node *lptr = listsrc->first;
	Node *nptr;

	//is the list we're copying empty
	if (lptr == NULL)
	{
		listdest->current = listdest->first = listdest->last = NULL;
		listdest->size = 0;
	}
	else
	{
	//create the first Node
		nptr = (Node*)tracemalloc("List_Copy #1", sizeof(Node));
		nptr->value = lptr->value;
		nptr->name = NAME(lptr->name);

		//check if the Node we're copying is the current Node of the other list
		if (lptr == listsrc->current)
			listdest->current = nptr;

		//set the first pointer
		listdest->first = nptr;

		//move on to the next Node to copy
		lptr = lptr->next;

		while(lptr)
		{
			//create the next Node
			nptr->next = (Node*)tracemalloc("List_Copy #2", sizeof(Node));
			nptr = nptr->next;
			nptr->value = lptr->value;
			nptr->name = NAME(lptr->name);

			//check if the Node we're copying is the current Node of the other list
			if (lptr == listsrc->current)
			listdest->current= nptr;

			//move on to the next Node to copy
			lptr = lptr->next;
		}

		//finish the last Node
		nptr->next = NULL;

		//set the last pointer
		listdest->last = nptr;

		//set the size of the new list
		listdest->size = listsrc->size;
	}
}

void List_Clear(List* list)
{
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
	//Construct a new Node
	Node *nptr = (Node*)tracemalloc("List_InsertBefore", sizeof(Node));

	if (list->size == 0)
	{
		nptr->value = e;
		nptr->name = NAME(theName);
		nptr->next = NULL;
		list->current = list->first = list->last = nptr;
	}
	else
	{
		//To avoid traversals, insert after current and then swap values
		nptr->value = list->current->value;
		nptr->next = list->current->next;
		nptr->name = list->current->name;
		list->current->value = e;
		list->current->next = nptr;
		list->current->name = NAME(theName);
		if (list->current == list->last)
			list->last = nptr;
	}
	list->size++;
}

void List_InsertAfter(List* list, void* e, LPCSTR theName)
{
	//Construct a new Node and fill it with the appropriate value
	Node *nptr = (Node*)tracemalloc("List_InsertAfter", sizeof(Node));
	nptr->value = e;
	nptr->name = NAME(theName);

	if (list->size == 0)
	{
		nptr->next = NULL;
		list->current = list->first = list->last = nptr;
	}
	else
	{
		nptr->next = list->current->next;
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
	else if (list->current == list->last)
	{
		nptr = list->first;
		while (nptr->next != list->current)
			nptr = nptr->next;

		nptr->next = NULL;
		Node_Clear(list->current);
		tracefree(list->current);

		list->current = list->last = nptr;
	}
	else
	{
		nptr = list->current->next;
		list->current->value = nptr->value;
		if(list->current->name)tracefree( (void*)list->current->name);
		list->current->name = NAME(nptr->name);
		list->current->next = nptr->next;
		if(list->last==nptr) list->last = list->current;
		Node_Clear(nptr);
		tracefree( (void*)nptr);
	}
	list->size--;
}

void List_GotoNext(List* list)
{
	if (list->current != list->last)
	list->current = list->current->next;
}

void List_GotoPrevious(List* list)
{
	if (list->current != list->first)
	{
		Node* nptr = list->first;
		while (nptr->next != list->current)
			nptr = nptr->next;
		list->current = nptr;
	}
}

void List_GotoLast(List* list)
{
	list->current = list->last;
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

	//This nasty chain of whiles and ifs is necessary because not all nodes
	//may actually be named.
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
	list->current = list->first;
}

int List_GetSize(const List* list)
{
	return list->size;
}

