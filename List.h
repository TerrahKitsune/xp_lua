#pragma once
#include <Windows.h>

typedef struct List {

	CRITICAL_SECTION CriticalSection;
	volatile size_t len;
	size_t alloc;
	void ** data;
}List;

//Enter the critical section
//Do this before enumerating the list in a multithreaded environment
void list_Enter(List * list);

//Leave the critical section
//Do this when enumeration is done in a mulithreaded environment
void list_Leave(List * list);

//Create a list
List * list_CreateList(size_t prealloc = 0);

//Add to the list
//returns -1 on failure otherwise it returns the index data was added at
int list_Add(List * list, void * data);

//Remove data from the list
//Returns 0 on failure
int list_Remove(List * list, void * data);

//Remove data by index
//Returns 0 on failure
int list_Remove(List * list, int index);

//Deallocate the list
void list_Destroy(List * list);