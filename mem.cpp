#include "mem.h"
#include <stdlib.h>
#include "MemoryManager.h"
#include "string.h"
#include <assert.h>
#include <math.h>
#include <windows.h>

#ifdef USEMEMORYMANAGER

static MemoryState * memState = NULL;

void * Allocate(size_t requested, size_t* actual) {

	*actual = requested;

	return HeapAlloc(GetProcessHeap(), 0, *actual);
}

void Deallocate(void * ptr) {

	assert(HeapFree(GetProcessHeap(), 0, ptr));
}

void * ReAllocate(void * ptr, size_t requested, size_t* actual) {

	*actual = requested;
	return HeapReAlloc(GetProcessHeap(), 0, ptr, *actual);
}

size_t EndMemoryManager() {

	assert(memState);
	size_t result = DestroyMemoryState(memState);
	memState = NULL;
	return result;
}

void InitMemoryManager() {

	assert(!memState);
	memState = CreateNewMemoryState(Allocate, Deallocate, ReAllocate);
}

void * gff_malloc(size_t size) {
	return MemoryStateAlloc(memState, size);
}

void * gff_calloc(size_t num, size_t size) {
	void*ptr = MemoryStateAlloc(memState, num*size);
	if (ptr) {
		memset(ptr, 0, num*size);
	}
	return ptr;
}

void * gff_realloc(void * ptr, size_t size) {
	return MemoryStateRealloc(memState, ptr, size);
}

void gff_free(void * ptr) {
	return MemoryStateDealloc(memState, ptr);
}

#elseifdef USEHEAPALLOC

size_t EndMemoryManager() {
	return 0;
}

void InitMemoryManager() {
}

void * gff_malloc(size_t size) {
	return HeapAlloc(GetProcessHeap(), 0, size);
}

void * gff_calloc(size_t num, size_t size) {
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, num*size);
}

void * gff_realloc(void * ptr, size_t size) {

	if (size == 0) {

		if (ptr) {
			assert(HeapFree(GetProcessHeap(), 0, ptr));
		}

		return NULL;
	}
	else if (!ptr) {
		return HeapAlloc(GetProcessHeap(), 0, size);
	}

	return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
}

void gff_free(void * ptr) {
	assert(HeapFree(GetProcessHeap(), 0, ptr));
}

#else

size_t EndMemoryManager() {
	return 0;
}

void InitMemoryManager() {
}

void * gff_malloc(size_t size) {

	return malloc(size);
}

void * gff_calloc(size_t num, size_t size) {

	return calloc(num, size);
}

void * gff_realloc(void * ptr, size_t size) {

	return realloc(ptr, size);
}

void gff_free(void * ptr) {
	free(ptr);
}

#endif