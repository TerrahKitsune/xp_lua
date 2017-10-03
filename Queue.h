#pragma once
#include <Windows.h>

typedef struct QueueEntry {

	void * Data;
	QueueEntry * Next;
}QueueEntry;

typedef struct Queue {

	CRITICAL_SECTION CriticalSection;
	QueueEntry * Root;
	QueueEntry * Last;
	volatile size_t Count;
}Queue;

Queue * queue_Create();
void queue_Destroy(Queue * q);
void queue_Enqueue(Queue * q, void * Data);
void * queue_Dequeue(Queue * q);
void * queue_Peek(Queue * q);
int queue_HasData(Queue * q);