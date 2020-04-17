#include "LinkedList.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 
#include "mem.h"

size_t Count(LLNode* head) {

	size_t cnt = 0;

	if (!head) {
		return cnt;
	}
	else {
		cnt++;
	}

	while (head->prev != NULL) {
		head = head->prev;
	}

	int nth = 0;
	for (LLNode* find = head; find->next; find = find->next) {
		cnt++;
	}

	return cnt;
}

LLNode* Remove(LLNode* node) {

	LLNode* find = node;

	if (node->next) {
		node->next->prev = node->prev;
	}

	if (node->prev) {
		node->prev->next = node->next;

		for (; find->prev; find = find->prev) {
		}
	}
	else {
		find = node->next;
	}

	Dealloc proc;

	if (node->data) {
		proc = (Dealloc)node->deallocfunc;
		proc(node->data);
	}

	gff_free(node);

	return find;
}

LLNode* Get(LLNode* head, int index) {

	if (index < 0 || !head) {
		return NULL;
	}

	while (head->prev != NULL) {
		head = head->prev;
	}

	if (index == 0) {
		return head;
	}

	int nth = 0;
	LLNode* find = head;
	for (; find->next; find = find->next) {
		if (nth++ == index) {
			return find;
		}
	}

	if (nth == index) {
		return find;
	}

	return NULL;
}

LLNode* Insert(LLNode* head, void* data, Dealloc* proc, int index) {

	LLNode* entry = (LLNode*)gff_calloc(1, sizeof(LLNode));

	if (!entry) {
		return NULL;
	}

	entry->data = data;

	if (proc) {
		entry->deallocfunc = proc;
	}
	else {
		entry->deallocfunc = (Dealloc*)&gff_free;
	}

	if (!head) {
		return entry;
	}

	while (head->prev) {
		head = head->prev;
	}

	int nth = 0;
	LLNode* find = NULL;
	LLNode* itr = head;
	for (; itr->next; itr = itr->next) {
		if (nth++ == index) {
			find = itr;
			break;
		}
	}

	if (nth == index) {
		find = itr;
	}

	if (find) {

		LLNode* temp = find->prev;

		if (!temp) {
			head = entry;
		}
		else {
			entry->prev = temp;
			temp->next = entry;
		}

		entry->next = find;
		find->prev = entry;
	}
	else {
		Dealloc proc;
		if (entry->data) {
			proc = (Dealloc)entry->deallocfunc;
			proc(entry->data);
		}

		gff_free(entry);
	}

	return head;
}

LLNode* AddBefore(LLNode* node, void* data, Dealloc* proc) {

	LLNode* entry = (LLNode*)gff_calloc(1, sizeof(LLNode));

	if (!entry) {
		return NULL;
	}

	entry->data = data;

	if (proc) {
		entry->deallocfunc = proc;
	}
	else {
		entry->deallocfunc = (Dealloc*)&gff_free;
	}

	if (!node) {
		return entry;
	}

	entry->next = node;
	entry->prev = node->prev;

	if (node->prev) {
		node->prev->next = entry;
	}

	node->prev = entry;

	while (node->prev) {
		node = node->prev;
	}

	return node;
}

LLNode* AddAfter(LLNode* node, void* data, Dealloc* proc) {

	LLNode* entry = (LLNode*)gff_calloc(1, sizeof(LLNode));

	if (!entry) {
		return NULL;
	}

	entry->data = data;

	if (proc) {
		entry->deallocfunc = proc;
	}
	else {
		entry->deallocfunc = (Dealloc*)&gff_free;
	}

	if (!node) {
		return entry;
	}

	entry->next = node->next;

	if (entry->next) {

		if (entry->next->prev) {
			entry->next->prev = entry;
		}

		entry->prev = node;
	}

	entry->prev = node;
	node->next = entry;
	
	while (node->prev) {
		node = node->prev;
	}

	return node;
}

LLNode* AddFirst(LLNode* head, void* data, Dealloc* proc) {

	LLNode* entry = (LLNode*)gff_calloc(1, sizeof(LLNode));

	if (!entry) {
		return NULL;
	}

	entry->data = data;

	if (proc) {
		entry->deallocfunc = proc;
	}
	else {
		entry->deallocfunc = (Dealloc*)&gff_free;
	}

	if (!head) {
		return entry;
	}

	entry->next = head;
	head->prev = entry;

	return entry;
}

LLNode* AddLast(LLNode* head, void* data, Dealloc* proc) {

	LLNode* entry = (LLNode*)gff_calloc(1, sizeof(LLNode));

	if (!entry) {
		return NULL;
	}

	entry->data = data;

	if (proc) {
		entry->deallocfunc = proc;
	}
	else {
		entry->deallocfunc = (Dealloc*)&gff_free;
	}

	if (!head) {
		return entry;
	}

	LLNode* find = head;

	for (; find->next; find = find->next) {

	}

	find->next = entry;
	entry->prev = find;

	return head;
}

void FreeLinkedList(LLNode* node) {

	if (!node) {
		return;
	}

	LLNode* find = node;
	for (; find->prev; find = find->prev) {
	}
	LLNode* temp;
	Dealloc proc;
	while (find) {

		if (find->data) {
			proc = (Dealloc)find->deallocfunc;
			proc(find->data);
		}

		temp = find;
		find = find->next;
		gff_free(temp);
	}
}