#include "luasocket.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 

int LuaSocketHasData(lua_State* L) {

	LuaSocket* socket = lua_toluasocket(L, 1);
	char d;

	lua_pop(L, lua_gettop(L));

	if (recv(socket->s, &d, 1, MSG_PEEK) == 1) {
		lua_pushboolean(L, true);
	}
	else if (WSAGetLastError() == WSAEWOULDBLOCK) {
		lua_pushboolean(L, false);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int LuaSocketReadData(lua_State* L) {

	LuaSocket* socket = lua_toluasocket(L, 1);

	if (socket->IsServer) {
		luaL_error(L, "Cannot read on listening socket");
		return 0;
	}

	size_t buffersize = (size_t)luaL_optinteger(L, 2, 1500);

	if (!socket->buf || socket->bufsize != buffersize) {

		if (socket->buf) {
			free(socket->buf);
		}

		socket->buf = (char*)malloc(buffersize);
		socket->bufsize = buffersize;
	}

	if (!socket->buf) {
		luaL_error(L, "Unable to allocate buffer");
	}

	lua_pop(L, lua_gettop(L));
	int d = recv(socket->s, socket->buf, buffersize, 0);

	if (d > 0) {
		lua_pushlstring(L, socket->buf, d);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int LuaSocketWrite(lua_State* L) {

	LuaSocket* socket = lua_toluasocket(L, 1);

	if (socket->IsServer) {
		luaL_error(L, "Cannot send on listening socket");
		return 0;
	}

	size_t len;
	const char* data = luaL_checklstring(L, 2, &len);

	int n = send(socket->s, data, len, 0);
	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, n);

	return 1;
}

int LuaSocketSelect(lua_State* L) {

	LuaSocket* socket = lua_toluasocket(L, 1);
	long timeout = luaL_optinteger(L, 2, 0);

	fd_set setR, setW, setE;

	FD_ZERO(&setW);
	FD_SET(socket->s, &setW);
	FD_ZERO(&setE);
	FD_SET(socket->s, &setE);
	FD_ZERO(&setR);
	FD_SET(socket->s, &setR);

	timeval time_out = { 0 };
	time_out.tv_sec = timeout;
	time_out.tv_usec = 0;

	lua_pop(L, lua_gettop(L));

	int ret = select(0, &setR, &setW, &setE, &time_out);

	lua_pushboolean(L, ret != SOCKET_ERROR);
	lua_pushboolean(L, FD_ISSET(socket->s, &setR));
	lua_pushboolean(L, FD_ISSET(socket->s, &setW));
	lua_pushboolean(L, FD_ISSET(socket->s, &setE));

	return 4;
}

int LuaSocketOpen(lua_State* L) {

	const char* addr = luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);
	int family = luaL_optinteger(L, 3, AF_INET);
	int socktype = luaL_optinteger(L, 4, SOCK_STREAM);
	int protocol = luaL_optinteger(L, 5, IPPROTO_TCP);

	struct addrinfo* result = NULL, * ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = socktype;
	hints.ai_protocol = protocol;

	char portstr[15];
	sprintf(portstr, "%d", port);

	int iResult = getaddrinfo(addr, portstr, &hints, &result);
	if (iResult != 0) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushfstring(L, "Unable to resolve address %s:%s", addr, portstr);

		return 2;
	}

	SOCKET s = INVALID_SOCKET;
	u_long flag = 1;

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (s == INVALID_SOCKET) {

			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushfstring(L, "Unable to create socket %s:%s (Error: %d)", addr, portstr, WSAGetLastError());

			if (result) {
				freeaddrinfo(result);
			}

			return 2;
		}

		iResult = connect(s, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {

			closesocket(s);
			s = INVALID_SOCKET;
			continue;
		}

		ioctlsocket(s, FIONBIO, &flag);

		break;
	}

	if (result) {
		freeaddrinfo(result);
	}

	if (s == INVALID_SOCKET) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushfstring(L, "Unable to open socket %s:%s (Error: %d)", addr, portstr, WSAGetLastError());

		return 2;
	}

	lua_pop(L, lua_gettop(L));
	LuaSocket* sock = lua_pushluasocket(L);

	sock->IsServer = false;
	sock->s = s;

	return 1;
}

int LuaSocketOpenListener(lua_State* L) {

	int port = luaL_checkinteger(L, 1);
	int family = luaL_optinteger(L, 2, AF_INET);
	int socktype = luaL_optinteger(L, 3, SOCK_STREAM);
	int protocol = luaL_optinteger(L, 4, IPPROTO_TCP);

	struct addrinfo* result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = socktype;
	hints.ai_protocol = protocol;

	char portstr[15];
	sprintf(portstr, "%d", port);

	int iResult = getaddrinfo(NULL, portstr, &hints, &result);
	if (iResult != 0) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushfstring(L, "Unable to resolve port %s", portstr);

		return 2;
	}

	SOCKET s = INVALID_SOCKET;
	u_long flag = 1;

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (s == INVALID_SOCKET) {

			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushfstring(L, "Unable to create socket %s (Error: %d)", portstr, WSAGetLastError());

			if (result) {
				freeaddrinfo(result);
			}

			return 2;
		}

		iResult = bind(s, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {

			closesocket(s);
			s = INVALID_SOCKET;
			continue;
		}

		iResult = listen(s, SOMAXCONN);
		if (iResult == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {

			closesocket(s);
			s = INVALID_SOCKET;
			continue;
		}

		ioctlsocket(s, FIONBIO, &flag);

		break;
	}

	if (result) {
		freeaddrinfo(result);
	}

	if (s == INVALID_SOCKET) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushfstring(L, "Unable to open socket %s (Error: %d)", portstr, WSAGetLastError());

		return 2;
	}

	lua_pop(L, lua_gettop(L));
	LuaSocket* sock = lua_pushluasocket(L);

	sock->IsServer = true;
	sock->s = s;

	return 1;
}

int LuaSocketAccept(lua_State* L) {

	LuaSocket* socket = lua_toluasocket(L, 1);

	if (!socket->IsServer) {
		luaL_error(L, "Cannot accept on client socket");
		return 0;
	}

	SOCKET s = accept(socket->s, NULL, NULL);

	lua_pop(L, lua_gettop(L));

	if (s == INVALID_SOCKET) {
		
		lua_pushnil(L);
	}
	else {

		LuaSocket* client = lua_pushluasocket(L);

		client->IsServer = false;
		client->s = s;
	}

	return 1;
}

int LuaSocketGetInfo(lua_State* L) {

	LuaSocket* socket = lua_toluasocket(L, 1);

	sockaddr_in6  client_info = { 0 };
	int len = sizeof(sockaddr_in6);

	if (socket->bufsize < 1024) {
		
		if (socket->buf) {
			free(socket->buf);
		}
		socket->bufsize = 0;

		socket->buf = (char*)malloc(1024);

		if (socket->buf) {
			socket->bufsize = 1024;
		}
		else {
			luaL_error(L, "Unable to allocate buffer");
			return 0;
		}
	}

	int result = getpeername(socket->s, (sockaddr*)&client_info, &len);

	lua_pop(L, lua_gettop(L));

	if (client_info.sin6_family == AF_INET6) {
		lua_pushstring(L, inet_ntop(client_info.sin6_family, &client_info.sin6_addr, socket->buf, socket->bufsize));
		lua_pushinteger(L, client_info.sin6_port);
	}
	else if (client_info.sin6_family == AF_INET) {
		sockaddr_in* ipv4 = (sockaddr_in*)(&client_info);
		lua_pushstring(L, inet_ntop(ipv4->sin_family, &ipv4->sin_addr, socket->buf, socket->bufsize));
		lua_pushinteger(L, ipv4->sin_port);
	}
	else {
		lua_pushnil(L);
	}

	lua_pushinteger(L, socket->s);

	return 3;
}

LuaSocket* lua_pushluasocket(lua_State* L) {

	LuaSocket* sock = (LuaSocket*)lua_newuserdata(L, sizeof(LuaSocket));

	if (sock == NULL)
		luaL_error(L, "Unable to push socket");

	luaL_getmetatable(L, LUASOCKET);
	lua_setmetatable(L, -2);

	memset(sock, 0, sizeof(LuaSocket));

	return sock;
}

LuaSocket* lua_toluasocket(lua_State* L, int index) {

	LuaSocket* sock = (LuaSocket*)luaL_checkudata(L, index, LUASOCKET);

	if (sock == NULL)
		luaL_error(L, "parameter is not a %s", LUASOCKET);

	return sock;
}

int luasocket_gc(lua_State* L) {

	LuaSocket* pipe = lua_toluasocket(L, 1);

	if (pipe->s != INVALID_SOCKET) {
		shutdown(pipe->s, SD_BOTH);
		closesocket(pipe->s);
		pipe->s = INVALID_SOCKET;
	}

	if (pipe->buf) {
		free(pipe->buf);
		pipe->buf = NULL;
	}

	return 0;
}

int luasocket_tostring(lua_State* L) {
	char tim[100];
	sprintf(tim, "Socket: 0x%08X", lua_toluasocket(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}