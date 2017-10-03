#include "Queue.h"

Queue * queue_Create() {

	Queue * queue = (Queue*)calloc(1, sizeof(Queue));

	InitializeCriticalSectionAndSpinCount(&queue->CriticalSection, 0x00000400);

	return queue;
}

void queue_Destroy(Queue * q) {

	EnterCriticalSection(&q->CriticalSection);

	QueueEntry * temp;
	QueueEntry * entry = q->Root;
	while (entry) {

		temp = entry;
		entry = entry->Next;

		free(temp);
	}

	LeaveCriticalSection(&q->CriticalSection);

	DeleteCriticalSection(&q->CriticalSection);

	free(q);
}

void queue_Enqueue(Queue * q, void * Data) {

	EnterCriticalSection(&q->CriticalSection);

	QueueEntry * temp = (QueueEntry*)calloc(1, sizeof(QueueEntry));

	if (!temp) {
		LeaveCriticalSection(&q->CriticalSection);
		return;
	}

	temp->Data = Data;

	if (q->Root) {
		q->Last->Next = temp;
	}
	else {
		q->Root = temp;
	}

	q->Last = temp;
	q->Count++;

	LeaveCriticalSection(&q->CriticalSection);
}

void * queue_Dequeue(Queue * q) {

	EnterCriticalSection(&q->CriticalSection);

	if (!q->Root) {
		LeaveCriticalSection(&q->CriticalSection);
		return NULL;
	}

	QueueEntry * temp = q->Root;
	q->Root = temp->Next;

	if (!q->Root) {
		q->Last = NULL;
		q->Count = 0;
	}
	else {
		q->Count--;
	}

	LeaveCriticalSection(&q->CriticalSection);

	void * data = temp->Data;

	free(temp);

	return data;
}

void * queue_Peek(Queue * q) {

	EnterCriticalSection(&q->CriticalSection);

	if (!q->Root)
		return NULL;

	void * data = q->Root->Data;
	
	LeaveCriticalSection(&q->CriticalSection);

	return data;
}

int queue_HasData(Queue * q) {

	if (!q->Root)
		return 0;

	return 1;
}