#pragma once

typedef void (*Dealloc)(void* data);

struct LLNode {
	void* data;
	struct LLNode* next;
	struct LLNode* prev;
	Dealloc* deallocfunc;
};

LLNode* AddFirst(LLNode* head, void* data, Dealloc* proc);
LLNode* AddLast(LLNode* head, void* data, Dealloc* proc);
LLNode* Get(LLNode* head, int index);
size_t Count(LLNode* head);
LLNode* Remove(LLNode* node);
LLNode* Insert(LLNode* head, void* data, Dealloc* proc, int index);

void FreeLinkedList(LLNode* node);