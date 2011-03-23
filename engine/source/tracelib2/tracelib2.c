#include "tracelib2.h"
#include "List.h"
#include <assert.h>

static List tlList;
static const char* TL_EOUTOFMEM = "error: out of memory";

void tlerror(char* msg, int line, char* file) {
	fprintf(stderr, "%s (%s:%d)\n", msg, file, line);
}


void* tlmalloc(int line, char* file, size_t size) {
	tlInfo* mem = NULL;
	if(!size) {
		fprintf(stderr, "tried to nallocate memory with size 0, called from %s:%d\n", file, line);
		return NULL;
	}
	mem = malloc(size + sizeof(tlInfo));
	if(!mem) {
		tlerror((char*) TL_EOUTOFMEM, line, file);
		return NULL;
	}
	mem->line = line;
	mem->file = file;
	mem->size = size;
	mem->buf = (char*) mem + sizeof(tlInfo);
	List_GotoLast(&tlList);
	List_InsertAfter(&tlList, (void*) mem, NULL);
	return (void*) mem->buf;
}

tlInfo* tlgetorigin(void* ptr) {
	return (void*)(((char*) ptr) - sizeof(tlInfo));
}

void tlfree(int line, char* file, void* ptr) {
	tlInfo* mem;
	if(!List_Includes(&tlList, tlgetorigin(ptr)) || !(mem = (tlInfo*) List_Retrieve(&tlList))) {
		fprintf(stderr, "tried to free unallocated memory at %p called from %s:%d\n", ptr, file, line);
		return;
	}
	List_Remove(&tlList);
	free(mem);
}

void* tlrealloc(int line, char* file, void* ptr, size_t size) {
	tlInfo* mem = NULL;
	tlInfo* old = tlgetorigin(ptr);
	if(!List_Includes(&tlList, old)) {
		fprintf(stderr, "tried to realloc unallocated memory at %p called from %s:%d\n", ptr, file, line);
		return NULL;
	}
	if(old->size >= size) {
		fprintf(stderr, "tried to realloc with lower size at %p called from %s:%d\n", ptr, file, line);
		return old->buf;
	}
	mem = realloc(old, size + sizeof(tlInfo));
	if(!mem) {
		tlerror((char*) TL_EOUTOFMEM, line, file);
		List_Remove(&tlList);
		return NULL;
	}
	mem->line = line;
	mem->file = file;
	mem->size = size;
	mem->buf = (char*) mem + sizeof(tlInfo);
	assert(old != mem);
	assert(tlList.current->value == (void*)old);
	List_Update(&tlList, (void*)mem);
	assert(tlList.current->value == (void*)mem);
	return (void*) mem->buf;
}

void* tlcalloc(int line, char* file, size_t count, size_t size) {
	tlInfo* mem = tlgetorigin(tlmalloc(line, file, count * size));
	if(mem == tlgetorigin(NULL))
		return NULL;
	memset(mem->buf,0,count*size);
	return (void*) mem->buf;
}

void tlinit(void) {
	List_Init(&tlList);
}

// prints stats and cleans up
// returns 0 if everything ok, else count of unfreed adresses
int tlstats(void) {
	tlInfo* mem;
	size_t save = List_GetSize(&tlList);
	printf("tracelib2 (R) simple mem debugging library (C) 2011 by anallyst (TM)\n");
	printf("====================================================================\n");
	List_GotoFirst(&tlList);
	while(List_GetCurrent(&tlList)) {
		mem = (tlInfo*) List_Retrieve(&tlList);
		assert(mem);
		assert(mem->size);
		printf("%d bytes still allocated from %s:%d\n", (int) mem->size, mem->file, mem->line);
		FREE(mem->buf);
	}
	List_Clear(&tlList);
	if(save) printf("====================================================================\n");
	printf("total number of unfreed chunks: %d\n", (int) save);
	return save;
}
