/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef LIST_H
#define LIST_H

// this switch enables the capability to create a hashmap for quick index lookups
// speeds up search for "contains" and "getindex" type of queries.
// it only consumes more ram if CreateIndices is actually called
// but it can be disabled to reduce binary size and mem usage. however the mentioned
// methods will be much slower.
#define USE_INDEX

// this switch enables the capability to create a hashmap for quick string searches
// speeds up search for "findByname" type of queries.
// it will always be used, so memory usage will be slightly increased.
#define USE_STRING_HASHES

#ifndef UNIT_TEST
#include "depends.h"
#endif

#include <stddef.h>
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

#define NAME(s) ((s==NULL)?NULL:(strcpy((char*)malloc(strlen(s)+1),s)))

typedef struct Node{
	struct Node* prev;          //pointer to previous Node
	struct Node* next;          //pointer to next Node
	void* value;                //data stored in a Node
	char* name;                //optional name of the Node
} Node;

#ifdef USE_INDEX
typedef struct LIndex {
	size_t size;
	size_t used;
	Node** nodes;
	ptrdiff_t* indices;
} LIndex;
#endif

#ifdef USE_STRING_HASHES
typedef struct Bucket {
	size_t size;
	size_t used;
	Node** nodes;
} Bucket;
#endif

typedef struct List {
	//Data members
	Node *first;
	Node *current;
	Node *last;
	void **solidlist;
	int index;
	int size;
#ifdef USE_INDEX
	LIndex** mindices;
#endif
#ifdef USE_STRING_HASHES
	Bucket** buckets;
#endif
#ifdef DEBUG
	int initdone;
#endif

} List;

void List_SetCurrent(List* list, Node* current);
void Node_Clear(Node* node);
void List_Init(List* list);
void List_Solidify(List* list);
int List_GetIndex(List* list);
void List_Copy(List* listdest, const List* listsrc);
void List_Clear(List* list);
void List_InsertBefore(List* list, void* e, char* theName);
void List_InsertAfter(List* list, void* e, char* theName);
void List_Remove(List* list);
int List_GotoNext(List* list);
int List_GotoPrevious(List* list);
int List_GotoLast(List* list);
int List_GotoFirst(List* list);
void* List_Retrieve(const List* list);
void* List_GetFirst(const List* list);
void* List_GetLast(const List* list);
void List_Update(List* list, void* e);
int List_Includes(List* list, void* e);
int List_FindByName(List* list, char* name);
char* List_GetName(const List* list);
void List_Reset(List* list);
int List_GetSize(const List* list);

Node* List_GetNodeByName(List* list, char* Name);
Node* List_GetNodeByValue(List* list, void* e);
Node* List_GetCurrentNode(List* list);
int List_GetNodeIndex(List* list, Node* node);
#ifdef USE_INDEX
void List_AddIndex(List* list, Node* node, size_t index);
void List_RemoveLastIndex(List* list);
void List_CreateIndices(List* list);
void List_FreeIndices(List* list);
unsigned char ptrhash(void* value); // need to export that as well for unittest.
#endif

#endif

