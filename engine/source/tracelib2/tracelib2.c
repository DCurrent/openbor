/*
 * tracelib2 library.
 * (C) 2011 anallyst
 *
 * license to use and redistribute is granted under the same terms as OpenBOR itself.
 * as long as anallyst is granted to use and redistribute the OpenBOR source code,
 * and maintain his own fork.
 * 
 */


#include "tracelib2.h"
#include "List.h"
#include <assert.h>

static List tlList;
static size_t tlTotalAlloced = 0;
static size_t tlTotalFreed = 0;
static size_t tlUsed = 0;
static size_t tlMaxUsed = 0;
static const char* TL_EOUTOFMEM = "error: out of memory";

void tladd(size_t size) {
	tlTotalAlloced += size;
	tlUsed += size;
	if(tlUsed > tlMaxUsed)
		tlMaxUsed = tlUsed;	
}

void tlrem(size_t size) {
	tlTotalFreed += size;
	tlUsed -= size;
}

void tlerror(char* msg, int line, char* file) {
	fprintf(stderr, "%s (%s:%d)\n", msg, file, line);
}

void tlsetmem(tlInfo* mem, int line, char* file, size_t size) {
	mem->line = line;
	mem->file = file;
	mem->size = size;
	mem->buf = (char*) mem + sizeof(tlInfo);	
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
	tladd(size);
	tlsetmem(mem, line, file, size);
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
	tlrem(mem->size);
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
	tlrem(old->size);
	if(!mem) {
		tlerror((char*) TL_EOUTOFMEM, line, file);
		List_Remove(&tlList);
		return NULL;
	}
	tladd(size);
	tlsetmem(mem, line, file, size);
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
	size_t saveTotalFreed = tlTotalFreed;
	size_t save = List_GetSize(&tlList);
	printf("tracelib2 (R) simple mem debugging library (C) 2011 by anallyst (TM)\n");
	printf("====================================================================\n");
	List_GotoFirst(&tlList);
	while(List_GetCurrentNode(&tlList)) {
		mem = (tlInfo*) List_Retrieve(&tlList);
		assert(mem);
		assert(mem->size);
		printf("%d bytes still allocated from %s:%d\n", (int) mem->size, mem->file, mem->line);
		FREE(mem->buf);
	}
	List_Clear(&tlList);
	if(save) printf("====================================================================\n");
	printf("total number of unfreed chunks: %d\n", (int) save);
	printf("total amount of allocated heap memory: %d\n", (int) tlTotalAlloced);
	printf("total amount of freed heap memory: %d\n", (int) saveTotalFreed);
	printf("peak memory usage: %d\n", (int) tlMaxUsed);
	return save;
}
