#include "Client.h"

Client * ClientConnect(const char * addr, int port) {

	Client * cli = (Client*)calloc(1, sizeof(Client));

	cli->s = INVALID_SOCKET;

	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	char portstr[15];
	sprintf(portstr, "%d", port);

	int iResult = getaddrinfo(addr, portstr, &hints, &result);
	if (iResult != 0) {
		free(cli);
		return NULL;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		cli->s = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (cli->s == INVALID_SOCKET) {
			cli->LastError = WSAGetLastError();
			return cli;
		}

		// Connect to server.
		iResult = connect(cli->s, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(cli->s);
			cli->s = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (cli->s == INVALID_SOCKET) {
		cli->LastError = WSAGetLastError();
		return cli;
	}

	u_long flag = 1;
	ioctlsocket(cli->s, FIONBIO, &flag);

	return cli;
}

size_t ClientReceive(Client * cli, char * buffer, size_t len, int * disconnected) {

	if (!cli || cli->s == INVALID_SOCKET || !buffer || len <= 0)
		return 0;

	int result = recv(cli->s, buffer, len, NULL);
	if (result <= 0) {

		cli->LastError = WSAGetLastError();
		if (result == 0 || cli->LastError != WSAEWOULDBLOCK) {
			if (disconnected)
				*disconnected = 1;

			shutdown(cli->s, SD_SEND);
			closesocket(cli->s);
			cli->s = INVALID_SOCKET;
		}

		return 0;
	}

	cli->LastError = 0;
	return (size_t)result;
}

size_t ClientSend(Client * cli, char * buffer, size_t len, int * disconnected) {

	if (!cli || cli->s == INVALID_SOCKET || !buffer || len <= 0)
		return 0;
	
	int result = send(cli->s, buffer, len, NULL);
	if (result <= 0) {

		cli->LastError = WSAGetLastError();
		if (result == 0 || cli->LastError != WSAEWOULDBLOCK) {
			if (disconnected)
				*disconnected = 1;

			shutdown(cli->s, SD_SEND);
			closesocket(cli->s);
			cli->s = INVALID_SOCKET;
		}

		return 0;
	}

	cli->LastError = 0;
	return (size_t)result;
}

void ClientDisconnect(Client * cli) {

	if (!cli)
		return;

	if (cli->s != INVALID_SOCKET) {
		shutdown(cli->s, SD_SEND);
		closesocket(cli->s);
		cli->s = INVALID_SOCKET;
	}

	free(cli);
}