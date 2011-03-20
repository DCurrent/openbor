#include "tracelib2.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
//gcc -Wall -g -DUNIT_TEST -I.. -I../gamelib -I../scriptlib ../scriptlib/List.c tracelib2.c tracelib2_test.c -o tracelib2_test
int main(void) {
	char* b1;
	char* b2;
	char* b3;
	size_t null = 0;
	void* nullptr = &null;
	
	tlinit();
	
	
	b1 = MALLOC(512);
	b2 = MALLOC(1012);
	FREE(b2);
	sprintf(b1, "hello world");
	b3 = MALLOC(strlen(b1));
	strcpy(b3, b1);
	
	b2 = REALLOC(b1, 1024);
	assert(b2 != b1);
	// should report invalid access
	FREE(b1);	
	assert(strcmp(b3, b2)==0);
	FREE(b2);
	
	b2 = CALLOC(1, sizeof(void*));
	assert(memcmp(b2, nullptr, sizeof(void*))==0);
	
	
	//should report b2 and b3 leaking
	return tlstats();	
}