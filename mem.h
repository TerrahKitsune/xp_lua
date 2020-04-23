#pragma once

void InitMemoryManager();
size_t EndMemoryManager();

void * gff_malloc(size_t size);
void * gff_calloc(size_t num, size_t size);
void * gff_realloc(void * ptr, size_t size);
void gff_free(void * ptr);
