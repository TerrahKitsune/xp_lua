#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "networking.h"
#include "List.h"

typedef struct Server{

	SOCKET ListenSocket;
	int Error;
	List * Clients;

}Server;

Server * CreateServer(int port);
SOCKET ServerAccept(Server  * srv);
size_t ServerReceive(Server * srv, SOCKET socket, char * buffer, size_t buffersize, int * Disconnected);
size_t ServerSend(Server * srv, SOCKET socket, char * buffer, size_t buffersize, int * Disconnected);
void ServerDisconnect(Server * srv, SOCKET socket);
void ServerShutdown(Server * srv);