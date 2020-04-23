#pragma once

typedef void * (*memoryBlockReAllocatorFunction) (void *ptr, size_t requestedSize, size_t* allocatedSize);
typedef void * (*memoryBlockAllocatorFunction) (size_t requestedSize, size_t* allocatedSize);
typedef void(*memoryBlockDeAllocatorFunction) (void *ptr);

typedef struct MemoryAlloc {

	void * ptr;
	size_t sizeRequested;
	size_t sizeAllocated;

	MemoryAlloc*next;

} MemoryAlloc;

typedef struct MemoryState {

	MemoryAlloc * alloc;
	size_t total;
	size_t allocations;
	size_t minBlockSize;

	memoryBlockAllocatorFunction allocator;
	memoryBlockDeAllocatorFunction deallocator;
	memoryBlockReAllocatorFunction reallocator;

} MemoryState;

MemoryState * CreateNewMemoryState(memoryBlockAllocatorFunction allocator, memoryBlockDeAllocatorFunction deallocator, memoryBlockReAllocatorFunction reallocator = nullptr);
size_t DestroyMemoryState(MemoryState * state);
void * MemoryStateAlloc(MemoryState * state, size_t size);
void * MemoryStateRealloc(MemoryState * state, void * ptr, size_t size);
void MemoryStateDealloc(MemoryState * state, void * ptr);