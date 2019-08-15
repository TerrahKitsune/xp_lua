#include "List.h"
#include <stdlib.h>
#include <string>

void list_Enter(List * list) {
	EnterCriticalSection(&list->CriticalSection);
}

void list_Leave(List * list) {
	LeaveCriticalSection(&list->CriticalSection);
}

List * list_CreateList(size_t prealloc) {

	if (prealloc < 10)
		prealloc = 10;

	List * list = (List*)malloc(sizeof(List));
	if (!list)
		return NULL;

	list->data = (void**)malloc(sizeof(void*) * prealloc);

	if (!list->data) {
		free(list);
		return NULL;
	}

	list->alloc = prealloc;
	list->len = 0;

	InitializeCriticalSectionAndSpinCount(&list->CriticalSection, 0x00000400);

	return list;
}

int list_Add(List * list, void * data) {

	EnterCriticalSection(&list->CriticalSection);

	if (list->len >= list->alloc) {
		void * temp = realloc(list->data, sizeof(void*) * (list->alloc + 10));
		if (!temp) {
			LeaveCriticalSection(&list->CriticalSection);
			return -1;
		}

		list->data = (void**)temp;
		list->alloc += 10;
	}

	int n = list->len;
	list->len++;

	list->data[n] = data;

	LeaveCriticalSection(&list->CriticalSection);

	return n;
}

int list_Remove(List * list, void * data) {

	EnterCriticalSection(&list->CriticalSection);

	int index = -1;

	for (unsigned int n = 0; n < list->len; n++) {
		if (list->data[n] == data) {
			index = n;
			break;
		}
	}

	LeaveCriticalSection(&list->CriticalSection);

	if (index < 0)
		return 0;

	return list_Remove(list, index);
}

int list_Remove(List * list, int index) {

	EnterCriticalSection(&list->CriticalSection);

	if (index < 0 || index >= (int)list->len)
		return 0;

	if (index == list->len - 1) {
		list->len--;
		return 1;
	}

	memmove(&list->data[index], &list->data[index + 1], (list->len - index) * sizeof(void*));

	list->len--;

	LeaveCriticalSection(&list->CriticalSection);

	return 1;
}

void list_Destroy(List * list) {

	DeleteCriticalSection(&list->CriticalSection);

	if (list->data)
		free(list->data);

	free(list);
}