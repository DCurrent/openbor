/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef LIST_H
#define LIST_H

#include "depends.h"
#include "tracemalloc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//A macro to simplify iterating through all this lists.
#define FOREACH( x, y ) { \
      size = List_GetSize(&x); \
      List_Reset(&x); \
      for (i = 0; i < size; i++){ \
         y \
         List_GotoNext(&x); \
      } \
   }

#define PFOREACH( x, y ) {\
   size = List_GetSize(x); \
   List_Reset(x); \
   for (i = 0; i < size; i++){ \
         y \
         List_GotoNext(x); \
      } \
   }

#define NAME(s) ((s==NULL)?NULL:(strcpy((CHAR*)tracemalloc("NAME(s)", strlen(s)+1),s)))

typedef struct Node{
	//struct Node* prev;          //pointer to previous Node
	struct Node* next;          //pointer to next Node	
	void* value;                //data stored in a Node
	LPCSTR name;                //optional name of the Node
} Node;

typedef struct List {
	//Data members
	Node *first;
	Node *current;
	Node *last;
	void **solidlist;
	int index;
	int size;
} List;


void Node_Clear(Node* node);
void List_Init(List* list);
void List_Solidify(List* list);
int List_GetIndex(List* list);
void List_Copy(List* listdest, const List* listsrc);
void List_Clear(List* list);
void List_InsertBefore(List* list, void* e, LPCSTR theName);
void List_InsertAfter(List* list, void* e, LPCSTR theName);
void List_Remove(List* list);
void List_GotoNext(List* list);
void List_GotoPrevious(List* list);
void List_GotoLast(List* list);
void* List_Retrieve(const List* list);
void* List_GetFirst(const List* list);
void* List_GetLast(const List* list);
void List_Update(List* list, void* e);
int List_Includes(List* list, void* e);
int List_FindByName(List* list, LPCSTR theName );
LPCSTR List_GetName(const List* list);
void List_Reset(List* list);
int List_GetSize(const List* list);
#endif

