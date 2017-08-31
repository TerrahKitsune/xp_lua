#pragma once
#include "networking.h"

typedef struct Client {

	int LastError;
	SOCKET s;

}Client;

Client * ClientConnect(const char * addr, int port);
size_t ClientReceive(Client * cli, char * buffer, size_t len, int * disconnected);
size_t ClientSend(Client * cli, char * buffer, size_t len, int * disconnected);
void ClientDisconnect(Client * cli);