/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <string.h>
#include <assert.h>
#include "models.h"
#include "List.h"
#include "utils.h"

static List* modellist;
static char convertbuf[1024];

void makelowercp(char* name) {
	assert(name);
	size_t len = strlen(name);
	assert(len < 1024);
	strncpy(convertbuf, name, 1024);
	lc(convertbuf, len);
}

void createModelList(void) {
	modellist = malloc(sizeof(List));
	List_Init(modellist);
}

void freeModelList(void) {
	List_Clear(modellist);
	free(modellist);
	modellist = NULL;
}

void addModel(s_model* model) {
	assert(model);
	assert(modellist);
	makelowercp(model->name);
	List_GotoLast(modellist);
	List_InsertAfter(modellist, (void*) model, convertbuf);
}

void deleteModel(char* modelname) {
	s_model* temp;
	assert(modellist);
	assert(modelname);
	makelowercp(modelname);
	if(List_FindByName(modellist, convertbuf) && (temp=List_Retrieve(modellist))) {
		List_Remove(modellist);
		free(temp);
	}
}

s_model* findmodel(char* modelname) {
	s_model* temp = NULL;
	assert(modellist);
	makelowercp(modelname);
	if(List_FindByName(modellist, convertbuf))
		temp=List_Retrieve(modellist);
	return temp;
}

s_model* getFirstModel(void) {
	assert(modellist);
	List_GotoFirst(modellist);
	return getCurrentModel();

}

s_model* getCurrentModel(void) {
	assert(modellist);
	Node* n = List_GetCurrentNode(modellist);
	if(n)
		return (s_model*) n->value;
	else
		return NULL;
}

s_model* getNextModel(void) {
	assert(modellist);
	List_GotoNext(modellist);
	return getCurrentModel();
}

int isLastModel(void) {
	if (!modellist->current || modellist->current == modellist->last)
		return 1;
	return 0;
}
