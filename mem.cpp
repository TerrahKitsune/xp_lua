#include "mem.h"
#include <stdlib.h>

#ifdef STANDALONEGFF

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

#else

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