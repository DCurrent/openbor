/*
 * test.c
 *
 *  Created on: Feb 27, 2011
 *      Author: anallyst
 *
 * set -DDEBUG when compiling List.c
 * to check access on non-initialized lists
 *
 gcc -Wall -DDEBUG -DNO_RAM_DEBUGGER -I.. -I../gamelib -g -O0 List_unittest.c List.c -o list_unittest

 */
#include "List.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void* memlist[1024];
size_t memidx = 0;

char* int2str (int i) {
	char * ret = malloc(32);
	sprintf(ret, "%d", i);
	memlist[memidx++] = ret;
	return ret;
}

void freemem() {
	size_t i;
	for (i = 0; i < memidx; i++) {
		free(memlist[i]);
	}
}

int eq (char* s1, char* s2) {
	return (strcmp(s1, s2) == 0);
}

int main() {

	List list, list2;
	size_t dummy = 0x1234;
	int i = 0;

	Node* test;

	List_Init(&list);
	assert(list.last == NULL);
	assert(list.first == NULL);
	assert(list.size == 0);
	assert(list.index == 0);


	List_InsertAfter(&list, (void*) dummy, int2str(++i));
	assert(list.current == list.first);
	assert(list.current == list.last);
	assert(list.last != NULL);
	assert(list.first != NULL);
	assert(list.last->next == NULL);
	assert(list.first->prev == NULL);

	assert(list.size == i);
	assert(eq(list.current->name, "1"));

	List_InsertAfter(&list, (void*) dummy, int2str(++i));
	assert(list.current->prev == list.first);
	assert(list.current == list.last);
	assert(list.last->next == NULL);
	assert(list.first->prev == NULL);
	assert(list.size == i);
	assert(eq(list.current->name, "2"));

	List_InsertAfter(&list, (void*) dummy, int2str(++i));
	assert(list.current->prev->prev == list.first);
	assert(list.current == list.last);
	assert(list.last->next == NULL);
	assert(list.first->prev == NULL);
	assert(list.size == i);
	assert(eq(list.current->name, "3"));

	List_Remove(&list);
	assert(list.current->prev == list.first);
	assert(list.current == list.last);
	assert(list.last->next == NULL);
	assert(list.first->prev == NULL);
	assert(list.size == i - 1);
	assert(eq(list.current->name, "2"));

	List_InsertBefore(&list, (void*) dummy, int2str(++i));
	assert(list.current != list.last);
	assert(list.current->next == list.last);

	assert(list.last->next == NULL);
	assert(list.first->prev == NULL);
	assert(list.size == i - 1);
	assert(eq(list.current->name, "4"));
	assert(eq(list.last->name, "2"));

	List_GotoFirst(&list);
	List_InsertBefore(&list, (void*) dummy, int2str(++i));
	assert(list.current == list.first);
	assert(list.last->next == NULL);
	assert(list.first->prev == NULL);
	assert(list.size == i - 1);
	assert(eq(list.current->name, "5"));
	assert(eq(list.last->name, "2"));

	List_InsertAfter(&list, (void*) dummy, int2str(++i));
	assert(list.current == list.first->next);
	assert(list.current->prev == list.first);
	assert(list.current->next->prev == list.current);
	assert(list.last->next == NULL);
	assert(list.first->prev == NULL);
	assert(list.size == i - 1);
	assert(eq(list.current->name, "6"));
	assert(eq(list.last->name, "2"));

	List_GotoFirst(&list);
	assert(list.current == list.first);
	test = list.first->next;
	List_Remove(&list);
	assert(list.first == test);
	assert(list.current == list.first);
	assert(list.last->next == NULL);
	assert(list.first->prev == NULL);
	assert(list.size == i - 2);
	//assert(eq(list.current->name, "7"));
	assert(eq(list.last->name, "2"));

	List_GotoFirst(&list);
	assert(list.current == list.first);

	List_Copy(&list2, &list);
	assert(list2.size == list.size);
	assert(list2.current == list2.first);
	assert(eq(list2.last->name, "2"));

	List_GotoLast(&list);
	assert(list.current == list.last);

	List_Copy(&list2, &list);
	assert(list2.current == list2.last);
	assert(list2.size == list.size);
	assert(list2.index == list.index);
	assert(list2.index == 0);
	assert(eq(list2.last->name, "2"));
	assert(eq(list2.last->name, list.last->name));
	assert(eq(list2.first->name, list.first->name));
	assert(eq(list2.current->name, list.current->name));
	assert(eq(list2.current->prev->name, list.current->prev->name));
	assert(list2.current->next == list.current->next);
	assert(list.current->next == 0);
	List_GotoFirst(&list);
	assert(!eq(list2.current->name, list.current->name));
	List_GotoFirst(&list2);
	assert(eq(list2.current->name, list.current->name));
	assert(list.current == list.first);
	assert(list.current->prev == NULL);
	assert(list2.current->prev == NULL);


	List_InsertAfter(&list, (void*) 0xDEADBEEF, int2str(++i));
	assert(list.first->next->value == (void*) 0xDEADBEEF);
	assert(list.current = list.first->next);

	List_InsertAfter(&list, (void*) dummy, int2str(++i));

	List_InsertBefore(&list, (void*) dummy, int2str(++i));
	List_InsertBefore(&list, (void*) dummy, int2str(++i));
	List_InsertBefore(&list, (void*) dummy, int2str(++i));
	assert(list.size == i - 2); // i = 11
	List_GotoFirst(&list);
	List_GotoNext(&list);
	List_GotoPrevious(&list);

	List_GotoLast(&list);

	test = list.last;
	assert(list.current == list.last);

	List_Remove(&list);
	assert(list.last != test);
	assert(list.current == list.last);

	List_Remove(&list);
	assert(list.current == list.last);

	List_Remove(&list);
	assert(list.current == list.last);

	List_Remove(&list);

	assert(list.current == list.last);
	assert(list.size == i - 6);

	List_GotoPrevious(&list);
	List_GotoPrevious(&list);
	List_GotoPrevious(&list);

	assert(List_Retrieve(&list) == (void*)0xDEADBEEF);
	List_Remove(&list);
	assert(!List_Includes(&list, (void*)0xDEADBEEF));
	assert(list.size == i - 7);

	List_GotoLast(&list);
	List_InsertAfter(&list, (void*) 0xDEADBEEF, int2str(++i));

	#ifdef USE_INDEX
	List_CreateIndices(&list);
	assert(list.mindices);
	assert(List_GetIndex(&list) == list.size -1);
	assert(List_GetNodeIndex(&list, list.first) == 0);
	assert(List_GetNodeIndex(&list, list.last->prev) == list.size -2);
	assert(List_GetNodeIndex(&list, list.last->prev->prev) == list.size -3);
	assert(list.mindices[ptrhash((void*)0xDEADBEEF)]);
	#endif
	assert(List_Includes(&list, (void*)0xDEADBEEF));

	assert(list.current == list.last);
	assert(List_Retrieve(&list) == (void*)0xDEADBEEF);

	List_Update(&list, (void*)0xDEADC0DE);
	assert(list.current == list.last);
	assert(List_Retrieve(&list) == (void*)0xDEADC0DE);
	assert(!List_Includes(&list, (void*)0xDEADBEEF));
	assert(List_Includes(&list, (void*)0xDEADC0DE));
	#ifdef USE_INDEX
	assert(list.mindices);
	assert(list.mindices[ptrhash((void*)0xDEADC0DE)]);
	#endif
	List_Remove(&list);
	assert(!List_Includes(&list, (void*)0xDEADC0DE));
	#ifdef USE_INDEX
	assert(list.mindices);
	assert(ptrhash((void*) dummy) != ptrhash((void*)0xDEADC0DE));
	assert(ptrhash((void*) dummy) != ptrhash((void*)0xDEADBEEF));
	// disable temporary
	assert(list.mindices[ptrhash((void*)0xDEADC0DE)]);
	assert(list.mindices[ptrhash((void*)0xDEADC0DE)]->nodes[0] == NULL);
	assert(list.mindices[ptrhash((void*)0xDEADC0DE)]->used ==0);
	// dead beef was updated, so used must be still 1
	assert(list.mindices[ptrhash((void*)0xDEADBEEF)]);
	assert(list.mindices[ptrhash((void*)0xDEADBEEF)]->nodes[0] == NULL);
	assert(list.mindices[ptrhash((void*)0xDEADBEEF)]->used == 1);
	#endif

	List_Clear(&list);
	assert(list.size == 0);
	List_Init(&list);
	i = 0;

	List_InsertBefore(&list, (void*) dummy, int2str(++i));
	List_InsertBefore(&list, (void*) dummy, int2str(++i));
	List_InsertBefore(&list, (void*) dummy, int2str(++i));
	assert(list.size == 3);
	List_Remove(&list);
	List_Remove(&list);
	List_Remove(&list);
	List_Remove(&list);
	List_GotoFirst(&list);

	List_Clear(&list);

	// testing the order of removal is equivalent to gotonext
	List_Init(&list);
	List_InsertAfter(&list, (void*) 1, NULL);
	List_InsertAfter(&list, (void*) 2, NULL);
	List_InsertAfter(&list, (void*) 3, NULL);
	List_InsertAfter(&list, (void*) 4, NULL);
	List_GotoFirst(&list);
	assert(list.current->value == (void*) 1);
	List_GotoNext(&list);
	assert(list.current->value == (void*) 2);
	List_GotoNext(&list);
	assert(list.current->value == (void*) 3);
	List_GotoNext(&list);
	assert(list.current->value == (void*) 4);
	assert(list.current == list.last);
	List_GotoFirst(&list);
	assert(list.current->value == (void*) 1);
	List_Remove(&list);
	assert(list.current->value == (void*) 2);
	List_Remove(&list);
	assert(list.current->value == (void*) 3);
	List_Remove(&list);
	assert(list.current->value == (void*) 4);
	assert(list.current == list.last);
	List_Remove(&list);
	assert(list.current == list.last);
	assert(list.current == list.first);
	assert(list.current == NULL);


	freemem();
	return 0;
}
