#include "LuaServer.h"
#include "LuaFrame.h"

static double PCFreq = 0.0;
static __int64 CounterStart = 0;
static double timeout = 1000.0;

static void StartCounter()
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		puts("QueryPerformanceFrequency failed!");

	PCFreq = double(li.QuadPart) / 1000.0;

	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;
}
static double GetCounter()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - CounterStart) / PCFreq;
}

static int CheckTimeout(lua_State *L){

	if (GetCounter() > timeout)
		return 1;

	return 0;
}

static bool AddClient(LuaServer * srv, SOCKET cli){

	if (!srv->Clients){
		srv->Clients = (SOCKET*)calloc(1, sizeof(SOCKET));
		if (!srv->Clients)
			return false;
		srv->Clients[0] = cli;
		srv->NumClients++;
		return true;
	}
	else{
		void * temp = realloc(srv->Clients, (srv->NumClients + 1) * sizeof(SOCKET));
		if (!temp)
			return false;

		srv->Clients = (SOCKET*)temp;
		srv->Clients[srv->NumClients] = cli;
		srv->NumClients++;
	}

	return true;
}

static bool RemoveClient(LuaServer * srv, SOCKET cli){

	if (!srv->Clients){
		return true;
	}

	if (srv->NumClients <= 1){

		free(srv->Clients);
		srv->Clients = NULL;
		srv->NumClients = 0;
		return true;
	}

	SOCKET * temp = (SOCKET*)calloc(srv->NumClients - 1, sizeof(SOCKET));
	if (!temp)
		return false;

	int added = 0;
	for (int n = 0; n < srv->NumClients; n++){
		if (srv->Clients[n] == cli)
			continue;

		temp[added++] = srv->Clients[n];
	}

	srv->NumClients--;
	free(srv->Clients);
	srv->Clients = temp;

	return true;
}

static bool Exists(LuaServer * srv, SOCKET cli){

	if (!srv->Clients)
		return false;
	else{
		for (int n = 0; n < srv->NumClients; n++){
			if (srv->Clients[n] == cli)
				return true;;
		}
	}
	return false;
}

LuaServer * lua_toluaserver(lua_State *L, int index){

	LuaServer * proc = (LuaServer*)lua_touserdata(L, index);
	if (proc == NULL)
		luaL_error(L, "parameter is not a %s", LUASERVER);
	return proc;
}

LuaServer * lua_pushluaserver(lua_State *L){

	LuaServer * proc = (LuaServer*)lua_newuserdata(L, sizeof(LuaServer));
	if (proc == NULL)
		luaL_error(L, "Unable to create LuaServer");
	luaL_getmetatable(L, LUASERVER);
	lua_setmetatable(L, -2);
	memset(proc, 0, sizeof(LuaServer));
	proc->Listener = INVALID_SOCKET;
	return proc;
}

int SvrStartLuaServer(lua_State *L){

	int port = luaL_checkinteger(L, 1);
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
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, iResult);
		return 2;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, WSAGetLastError());
		freeaddrinfo(result);
		return 2;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		return 2;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, WSAGetLastError());
		closesocket(ListenSocket);
		return 2;
	}

	u_long flag = 1;
	ioctlsocket(ListenSocket, FIONBIO, &flag);

	lua_pop(L, lua_gettop(L));
	LuaServer * srv = lua_pushluaserver(L);
	srv->Listener = ListenSocket;

	return 1;
}

int SvrAcceptConnection(lua_State *L){

	LuaServer * srv = lua_toluaserver(L, 1);
	SOCKET ClientSocket = accept(srv->Listener, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, WSAGetLastError());
		return 2;
	}

	if (!Exists(srv, ClientSocket)){
		if (!AddClient(srv, ClientSocket)){
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushinteger(L, WSAGetLastError());
			shutdown(ClientSocket, SD_SEND);
			closesocket(ClientSocket);
			return 2;
		}
		else{
			u_long flag = 1;
			ioctlsocket(ClientSocket, FIONBIO, &flag);
		}
	}

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, ClientSocket);
	return 1;
}

int SvrRecv(lua_State *L){

	LuaServer * srv = lua_toluaserver(L, 1);
	SOCKET target = luaL_checkinteger(L, 2);
	int read;

	if (!Exists(srv, target)){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, 1);
		return 2;
	}

	StartCounter();
	LuaFrame Frame;
	int result = recv(target, (char*)&Frame, sizeof(LuaFrame), 0);
	if (result == 0){
	disconnect:
		RemoveClient(srv, target);
		shutdown(target, SD_SEND);
		closesocket(target);
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, 1);
		return 2;
	}
	else if (result > 0){

		if (result < sizeof(LuaFrame) || Frame.length < sizeof(LuaFrame)){
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushinteger(L, WSAEWOULDBLOCK);
			return 2;
		}

		char * data = (char*)malloc(Frame.length);
		if (!data){
			goto error;
		}

		memcpy(data, &Frame, result);
		read = result;
		while (read < Frame.length){
			result = recv(target, &data[read], Frame.length - read, 0);
			if (result == 0){
				free(data);
				goto disconnect;
			}
			else if (result > 0){
				read += result;
			}
			else if (WSAGetLastError() == WSAEWOULDBLOCK){

				result = CheckTimeout(L);
				if (result == 0){
					Sleep(1);
					continue;
				}

				if (result != 1){
					RemoveClient(srv, target);
					shutdown(target, SD_SEND);
					closesocket(target);
				}
				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				lua_pushinteger(L, result);
				return 2;
			}
			else{
				free(data);
				goto error;
			}
		}

		lua_pop(L, lua_gettop(L));
		for (int n = 0; n < 16; n++){
			if (n == 15 || ((LuaFrame*)data)->method[n] == '\0'){
				lua_pushlstring(L, ((LuaFrame*)data)->method, n);
				break;
			}
		}
		lua_pushframe(L, (LuaFrame*)data);
		free(data);
	}
	else{
	error:
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushinteger(L, WSAGetLastError());
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			RemoveClient(srv, target);
			shutdown(target, SD_SEND);
			closesocket(target);
		}
		return 2;
	}

	return 2;
}

int SvrSend(lua_State *L){

	LuaServer * srv = lua_toluaserver(L, 1);
	SOCKET target = luaL_checkinteger(L, 2);

	if (!Exists(srv, target)){
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushinteger(L, 1);
		return 2;
	}


	LuaFrame * frame = lua_toframe(L, 4, lua_tostring(L, 3));
	if (!frame){
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushinteger(L, 0);
		return 2;
	}
	int sent = 0;
	char* buffer = (char*)frame;
	StartCounter();
	int result;
	while (sent < frame->length){
		result = send(target, &buffer[sent], frame->length - sent, 0);
		if (result == 0){
			RemoveClient(srv, target);
			shutdown(target, SD_SEND);
			closesocket(target);
			free(frame);
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushinteger(L, 1);
			return 2;
		}
		else if (result > 0){
			sent += result;
		}
		else if (WSAGetLastError() == WSAEWOULDBLOCK){
			result = CheckTimeout(L);
			if (result == 0){
				Sleep(1);
				continue;
			}

			if (result != 1){
				RemoveClient(srv, target);
				shutdown(target, SD_SEND);
				closesocket(target);
			}

			free(frame);
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushinteger(L, result);
			return 2;
		}
		else{
			free(frame);
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushinteger(L, WSAGetLastError());
			RemoveClient(srv, target);
			shutdown(target, SD_SEND);
			closesocket(target);
			return 2;
		}
	}
	free(frame);
	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int SvrGetClients(lua_State *L){

	LuaServer * srv = lua_toluaserver(L, 1);

	lua_createtable(L, srv->NumClients, 0);

	for (unsigned int n = 0; n < srv->NumClients; n++){
		lua_pushinteger(L, srv->Clients[n]);
		lua_rawseti(L, -2, n + 1);
	}

	return 1;
}

int SrvGetIP(lua_State *L){

	LuaServer * srv = lua_toluaserver(L, 1);
	SOCKET target = luaL_checkinteger(L, 2);
	lua_pop(L, lua_gettop(L));

	if (!Exists(srv, target)){
		lua_pushnil(L);
		return 1;
	}
	sockaddr res = { 0 };
	int size = sizeof(sockaddr);
	if (getpeername(target, (sockaddr*)&res, &size) != 0){
		lua_pushnil(L);
	}
	else{
		char *s = NULL;
		int port = 0;
		switch (res.sa_family) {
		case AF_INET: {
			struct sockaddr_in *addr_in = (struct sockaddr_in *)&res;
			port = addr_in->sin_port;
			s = (char*)malloc(INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &(addr_in->sin_addr), s, INET_ADDRSTRLEN);
			break;
		}
		case AF_INET6: {
			struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&res;
			port = addr_in6->sin6_port;
			s = (char*)malloc(INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, &(addr_in6->sin6_addr), s, INET6_ADDRSTRLEN);
			break;
		}
		default:
			break;
		}
		if (s){
			lua_pushstring(L, s);
			lua_pushinteger(L, port);
			free(s);
			return 2;
		}
		else
			lua_pushnil(L);
	}

	return 1;
}

int luaserver_gc(lua_State *L){

	LuaServer * srv = lua_toluaserver(L, 1);
	if (srv->Listener != INVALID_SOCKET){
		closesocket(srv->Listener);
		srv->Listener = INVALID_SOCKET;
	}

	if (srv->Clients)
	{
		for (int n = 0; n < srv->NumClients; n++){
			shutdown(srv->Clients[n], SD_SEND);
			closesocket(srv->Clients[n]);
		}

		free(srv->Clients);
		srv->Clients = NULL;
		srv->NumClients = 0;
	}

	lua_pop(L, 1);
	return 0;
}

int luaserver_tostring(lua_State *L){
	char tim[100];
	sprintf(tim, "LuaServer: 0x%08X", lua_toluaserver(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}