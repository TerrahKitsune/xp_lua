#pragma once
#include "networking.h"

#define NETEVENT_CONNECTED 1
#define NETEVENT_DISCONNECTED 2
#define NETEVENT_SEND 3
#define NETEVENT_RECEIVE 4

typedef struct NetEvent {

	SOCKET s;
	int type;
	int len;
	char data[];
};

NetEvent * NetEvent_Create(SOCKET s, int type, const char * data, int len);