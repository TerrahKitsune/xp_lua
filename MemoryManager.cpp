#include "MemoryManager.h"
#include <string.h>
#include <assert.h>

MemoryState * CreateNewMemoryState(memoryBlockAllocatorFunction allocator, memoryBlockDeAllocatorFunction deallocator, memoryBlockReAllocatorFunction reallocator) {

	size_t actual;
	size_t requestedSize = sizeof(MemoryState) + sizeof(MemoryAlloc);
	void * initialBlock = allocator(requestedSize, &actual);

	if (!initialBlock) {
		return NULL;
	}

	memset(initialBlock, 0, requestedSize);

	MemoryAlloc* alloc = (MemoryAlloc*)initialBlock;
	MemoryState* state = (MemoryState*)((char*)initialBlock + sizeof(MemoryAlloc));

	state->allocator = allocator;
	state->deallocator = deallocator;
	state->reallocator = reallocator;

	alloc->ptr = state;
	alloc->sizeRequested = requestedSize;
	alloc->sizeAllocated = actual - sizeof(MemoryAlloc);
	state->alloc = alloc;

	state->allocations++;
	state->total += actual;

	return state;
}

size_t DestroyMemoryState(MemoryState * state) {

	size_t leaked = 0;

	MemoryAlloc * alloc = state->alloc;
	MemoryAlloc * next;
	while (alloc) {

		next = alloc->next;
		if (alloc->ptr != (void*)state) {
			leaked++;
		}
		state->deallocator(alloc);
		alloc = next;
	}

	return leaked;
}

void _AddToState(MemoryState * state, MemoryAlloc * alloc) {

	alloc->next = state->alloc;
	state->alloc = alloc;
}

MemoryAlloc * _RemoveFromState(MemoryState * state, void * ptr) {

	MemoryAlloc * temp = NULL;

	if (state->alloc && state->alloc->ptr == ptr) {
		temp = state->alloc;
		state->alloc = temp->next;
		return temp;
	}

	for (MemoryAlloc* alloc = state->alloc; alloc; alloc = alloc->next)
	{
		if (alloc->ptr == ptr) {
			temp->next = alloc->next;
			return alloc;
		}
		else {
			temp = alloc;
		}
	}

	return NULL;
}

void * MemoryStateAlloc(MemoryState * state, size_t size) {

	size_t actual;
	size_t request = size + sizeof(MemoryAlloc);

	if (state->minBlockSize > 0) {
		if (request < state->minBlockSize) {
			request = state->minBlockSize;
		}
		else {
			request = request + (state->minBlockSize - (request % state->minBlockSize));
		}
	}

	void * ptr = state->allocator(request, &actual);

	if (!ptr) {
		return NULL;
	}

	state->allocations++;
	state->total += actual;

	MemoryAlloc * newAlloc = (MemoryAlloc *)ptr;
	newAlloc->ptr = ((char*)ptr + sizeof(MemoryAlloc));
	newAlloc->sizeRequested = size;
	newAlloc->sizeAllocated = actual - sizeof(MemoryAlloc);
	
	_AddToState(state, newAlloc);

	return newAlloc->ptr;
}

void * MemoryStateRealloc(MemoryState * state, void * ptr, size_t size) {

	if (size <= 0) {

		if (ptr) {
			MemoryStateDealloc(state, ptr);
		}

		return NULL;
	}
	else if (!ptr) {
		return MemoryStateAlloc(state, size);
	}

	MemoryAlloc*toRealloc = _RemoveFromState(state, ptr);

	assert(toRealloc);
	void * newMem;

	if (toRealloc->sizeAllocated >= size) {
		toRealloc->sizeRequested = size;
		_AddToState(state, toRealloc);
		return toRealloc->ptr;
	}
	else if (state->reallocator) {

		void* original = (void*)toRealloc;

		size_t request = size + sizeof(MemoryAlloc);

		if (state->minBlockSize > 0) {
			if (request < state->minBlockSize) {
				request = state->minBlockSize;
			}
			else {
				request = request + (state->minBlockSize - (request % state->minBlockSize));
			}
		}

		size_t actual;
		newMem = state->reallocator(original, request, &actual);

		if (!newMem) {
			return NULL;
		}
		else if (newMem == original) {

			state->total += (actual - (toRealloc->sizeAllocated + sizeof(MemoryAlloc)));

			toRealloc->sizeAllocated = actual - sizeof(MemoryAlloc);
			toRealloc->sizeRequested = size;

			_AddToState(state, toRealloc);

			return toRealloc->ptr;
		}
		else {

			toRealloc = (MemoryAlloc*)newMem;

			state->total += (actual - (toRealloc->sizeAllocated + sizeof(MemoryAlloc)));
		
			toRealloc->sizeAllocated = actual - sizeof(MemoryAlloc);
			toRealloc->sizeRequested = size;		
			toRealloc->ptr = ((char*)newMem + sizeof(MemoryAlloc));

			_AddToState(state, toRealloc);

			return toRealloc->ptr;
		}
	}
	else {
		newMem = MemoryStateAlloc(state, size);
		
		if (!newMem) {
			return NULL;
		}

		memcpy(newMem, toRealloc->ptr, toRealloc->sizeRequested);
		MemoryStateDealloc(state, ptr);
		return newMem;
	}
}

void MemoryStateDealloc(MemoryState * state, void * ptr) {

	if (!ptr) {
		return;
	}

	MemoryAlloc*toDie = _RemoveFromState(state, ptr);

	assert(toDie);

	state->allocations--;
	state->total -= toDie->sizeAllocated + sizeof(MemoryAlloc);

	state->deallocator(toDie);
}