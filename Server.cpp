#include "Server.h"

Server * CreateServer(int port) {

	Server * r = (Server*)calloc(1, sizeof(Server));

	r->ListenSocket = INVALID_SOCKET;
	r->Clients = list_CreateList();

	SOCKET ListenSocket;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	char portstr[15];
	sprintf(portstr, "%d", port);

	// Resolve the server address and port
	int iResult = getaddrinfo(NULL, portstr, &hints, &result);
	if (iResult != 0) {
		return r;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		freeaddrinfo(result);
		return r;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);

	freeaddrinfo(result);

	if (iResult == SOCKET_ERROR) {
		r->Error = WSAGetLastError();
		closesocket(ListenSocket);
		return r;
	}

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		r->Error = WSAGetLastError();
		closesocket(ListenSocket);
		return r;
	}

	u_long flag = 1;
	ioctlsocket(ListenSocket, FIONBIO, &flag);

	r->ListenSocket = ListenSocket;

	return r;
}

SOCKET ServerAccept(Server  * srv) {

	if (!srv || srv->ListenSocket == INVALID_SOCKET)
		return INVALID_SOCKET;

	SOCKET result = accept(srv->ListenSocket, NULL, NULL);
	if (result == INVALID_SOCKET) {

		srv->Error = WSAGetLastError();
		return INVALID_SOCKET;
	}
	else
		srv->Error = 0;

	list_Enter(srv->Clients);

	for (unsigned int n = 0; n < srv->Clients->len; n++) {

		if (*(SOCKET*)srv->Clients->data[n] == result) {
			list_Leave(srv->Clients);
			shutdown(result, SD_SEND);
			closesocket(result);
			return INVALID_SOCKET;
		}
	}

	list_Leave(srv->Clients);

	SOCKET * data = (SOCKET*)malloc(sizeof(SOCKET));
	memcpy(data, &result, sizeof(SOCKET));
	list_Add(srv->Clients, data);

	u_long flag = 1;
	ioctlsocket(*data, FIONBIO, &flag);

	return *data;
}

size_t ServerReceive(Server * srv, SOCKET socket, char * buffer, size_t buffersize, int * Disconnected) {

	if (!srv || !srv->Clients || socket == INVALID_SOCKET)
		return 0;

	SOCKET* real = NULL;

	list_Enter(srv->Clients);

	for (unsigned int n = 0; n < srv->Clients->len; n++) {

		if (*(SOCKET*)srv->Clients->data[n] == socket) {
			real = (SOCKET*)&srv->Clients->data[n];
			break;
		}
	}

	list_Leave(srv->Clients);

	if (real == NULL) {
		return 0;
	}
	
	int result = recv(socket, buffer, buffersize, 0);

	if (result <= 0) {

		srv->Error = WSAGetLastError();

		if ((result == 0 || srv->Error != WSAEWOULDBLOCK) && list_Remove(srv->Clients, real)) {
			shutdown(*real, SD_SEND);
			closesocket(*real);
			free(real);

			if (Disconnected)
				*Disconnected = 1;
		}
		
		return 0;
	}

	srv->Error = 0;

	return (size_t)result;
}

size_t ServerSend(Server * srv, SOCKET socket, char * buffer, size_t buffersize, int * Disconnected) {

	if (!srv || !srv->Clients || socket == INVALID_SOCKET)
		return 0;

	SOCKET* real = NULL;

	list_Enter(srv->Clients);

	for (unsigned int n = 0; n < srv->Clients->len; n++) {

		if (*(SOCKET*)srv->Clients->data[n] == socket) {
			real = (SOCKET*)srv->Clients->data[n];
			break;
		}
	}

	list_Leave(srv->Clients);

	if (real == NULL) {
		return 0;
	}

	int result = send(socket, buffer, buffersize, 0);

	if (result <= 0) {

		srv->Error = WSAGetLastError();

		if ((result == 0 || srv->Error != WSAEWOULDBLOCK) && list_Remove(srv->Clients, real)) {
			shutdown(*real, SD_SEND);
			closesocket(*real);
			free(real);

			if (Disconnected)
				*Disconnected = 1;
		}

		return 0;
	}

	srv->Error = 0;

	return (size_t)result;

}

void ServerDisconnect(Server * srv, SOCKET socket) {

	if (!srv || !srv->Clients || socket == INVALID_SOCKET)
		return;

	SOCKET* real = NULL;

	list_Enter(srv->Clients);

	for (unsigned int n = 0; n < srv->Clients->len; n++) {

		if (*(SOCKET*)srv->Clients->data[n] == socket) {
			real = (SOCKET*)srv->Clients->data[n];
			break;
		}
	}

	list_Leave(srv->Clients);

	if (!real)
		return;

	if (list_Remove(srv->Clients, real)) {
		shutdown(*real, SD_SEND);
		closesocket(*real);
		free(real);
	}
}

void ServerShutdown(Server * srv) {

	if (!srv)
		return;

	list_Enter(srv->Clients);

	for (unsigned int n = 0; n < srv->Clients->len; n++) {

		if (srv->Clients->data[n] != NULL) {
			free(srv->Clients->data[n]);
			srv->Clients->data[n] = NULL;
		}
	}

	list_Leave(srv->Clients);

	list_Destroy(srv->Clients);

	free(srv);
}