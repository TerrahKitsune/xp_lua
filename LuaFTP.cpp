#include "LuaFTP.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 

#define FTPBUFFERSIZE 4096
char FTPBUFFER[FTPBUFFERSIZE];
time_t FTPTIMEOUT = 1;

int ftp_recv(SOCKET s, char* buffer, size_t len, time_t timeout) {

	int result;

	size_t offset;
	time_t timestamp = time(NULL);

	for (offset = 0; offset < len; offset++)
	{
		result = recv(s, &buffer[offset], 1, 0);

		while (result == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {

			if (time(NULL) - timestamp >= timeout) {
				return -1;
			}

			Sleep(1);
			result = recv(s, &buffer[offset], 1, 0);
		}

		if (result < 0) {

			sprintf(buffer, "%s", "Recv error: %d", WSAGetLastError());
			return 0;
		}
		else if (buffer[offset] == '\n') {

			break;
		}
	}

	buffer[offset] = '\0';

	return atoi(buffer);
}

int ftp_send(SOCKET s, const char* buffer, size_t len, time_t timeout) {

	size_t sent = 0;
	int ret;
	time_t timestamp = time(NULL);

	do {

		ret = send(s, &buffer[sent], len - sent, 0);

		if (ret > 0) {
			sent += ret;
		}
		else if (ret == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {

			if (time(NULL) - timestamp >= timeout) {
				return 0;
			}
		}
		else {
			return 0;
		}

	} while (len - sent < 0);

	return 1;
}

size_t LuaAddMessagesToTable(lua_State* L, SOCKET s, int messageLog, time_t timeout) {

	int result = ftp_recv(s, FTPBUFFER, FTPBUFFERSIZE, timeout);

	if (result <= 0) {
		return 0;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, messageLog);

	size_t len = lua_rawlen(L, -1);
	size_t added = len + 1;

	while (result > 0) {

		lua_pushstring(L, FTPBUFFER);
		lua_rawseti(L, -2, ++len);

		result = ftp_recv(s, FTPBUFFER, FTPBUFFERSIZE, 0);
	}

	lua_pop(L, 1);

	return added;
}

int LuaGetLastMessage(lua_State* L, int messageLog, int idx) {

	lua_rawgeti(L, LUA_REGISTRYINDEX, messageLog);
	lua_rawgeti(L, -1, idx);

	size_t len;
	const char* data = lua_tolstring(L, -1, &len);

	strcpy(FTPBUFFER, data);

	lua_pop(L, 2);

	return atoi(FTPBUFFER);
}

size_t LuaGetLogSize(lua_State* L, int messageLog) {

	lua_rawgeti(L, LUA_REGISTRYINDEX, messageLog);

	size_t len = lua_rawlen(L, -1);

	lua_pop(L, 1);

	return len;
}

int LuaLogin(lua_State* L) {

	LuaFTP* ftp = lua_toluaftp(L, 1);
	const char* user = luaL_checkstring(L, 2);
	const char* pass = luaL_checkstring(L, 3);

	if (ftp->s == INVALID_SOCKET) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Not connected");

		return 2;
	}

	sprintf(FTPBUFFER, "USER %s\r\n", user);
	if (!ftp_send(ftp->s, FTPBUFFER, strlen(FTPBUFFER), FTPTIMEOUT)) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Connection timedout");

		return 2;
	}

	size_t msg = LuaAddMessagesToTable(L, ftp->s, ftp->log, FTPTIMEOUT);
	int result = LuaGetLastMessage(L, ftp->log, msg);

	if (result != 331) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushfstring(L, "Connection failed: %d", result);

		return 2;
	}

	sprintf(FTPBUFFER, "PASS %s\r\n", pass);
	if (!ftp_send(ftp->s, FTPBUFFER, strlen(FTPBUFFER), FTPTIMEOUT)) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Connection timedout");

		return 2;
	}

	msg = LuaAddMessagesToTable(L, ftp->s, ftp->log, FTPTIMEOUT);
	result = LuaGetLastMessage(L, ftp->log, msg);

	if (result != 230) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushfstring(L, "Connection failed: %d", result);

		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int LuaSetTimeout(lua_State* L) {

	int tm = luaL_checkinteger(L, 1);

	if (tm < 1) {
		tm = 1;
	}

	FTPTIMEOUT = tm;

	lua_pop(L, lua_gettop(L));

	return 0;
}

int LuaOpenDataChannel(lua_State* L) {

	LuaFTP* ftp = lua_toluaftp(L, 1);
	const char* addr = luaL_checkstring(L, 2);
	int port = luaL_checkinteger(L, 3);

	if (!lua_isfunction(L, 4)) {

		luaL_error(L, "Need to provide function");
		return 0;
	}

	SOCKET s = INVALID_SOCKET;
	char portstr[15];
	struct addrinfo* result = NULL, * ptr = NULL, hints;

	sprintf(portstr, "%d", port);

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int iResult = getaddrinfo(addr, portstr, &hints, &result);

	if (iResult != 0) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushfstring(L, "Unable to resolve address %s:%s", addr, portstr);

		return 2;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (s == INVALID_SOCKET) {

			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
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

		break;
	}

	if (result) {
		freeaddrinfo(result);
	}

	if (s == INVALID_SOCKET) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushfstring(L, "Unable to open socket %s:%s (Error: %d)", addr, portstr, WSAGetLastError());

		return 2;
	}

	int ret;
	u_long flag = 1;
	ioctlsocket(s, FIONBIO, &flag);
	const char* data;
	size_t datalen;
	time_t timer = time(NULL);
	bool worked = false;

	while (true) {

		LuaAddMessagesToTable(L, ftp->s, ftp->log, FTPTIMEOUT);

		worked = false;

		ret = recv(s, FTPBUFFER, FTPBUFFERSIZE, 0);

		if (ret < 0) {

			if (WSAGetLastError() != WSAEWOULDBLOCK) {

				closesocket(s);

				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				lua_pushfstring(L, "Error: %d", WSAGetLastError());

				return 2;
			}
		}
		else if (ret > 0) {
			worked = true;
		}

		lua_pushvalue(L, 4);
		lua_pushlstring(L, FTPBUFFER, ret <= 0 ? 0 : ret);

		ret = lua_pcall(L, 1, 2, 0);

		if (ret != 0) {

			closesocket(s);

			lua_pushboolean(L, false);
			lua_copy(L, -1, 1);
			lua_copy(L, -2, 2);
			lua_pop(L, lua_gettop(L) - 2);

			return 2;
		}
		else if (!lua_isboolean(L, -2) || !lua_toboolean(L, -2)) {

			closesocket(s);

			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, true);

			return 1;
		}
		else if (!lua_isnoneornil(L, -1)) {

			data = lua_tolstring(L, -1, &datalen);

			if (data && datalen > 0) {
				if (!ftp_send(s, data, datalen, FTPTIMEOUT)) {

					closesocket(s);
					lua_pop(L, lua_gettop(L));
					lua_pushboolean(L, false);
					lua_pushstring(L, "Send timeout");

					return 2;
				}
				else {
					worked = true;
				}
			}
		}

		lua_pop(L, 2);

		if (!worked) {

			if (time(NULL) - timer > FTPTIMEOUT) {

				closesocket(s);
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				lua_pushstring(L, "Send timeout");

				return 2;
			}
			else {
				Sleep(1);
			}
		}
		else {
			timer = time(NULL);
		}
	}

	return 0;
}

int LuaPassive(lua_State* L) {

	LuaFTP* ftp = lua_toluaftp(L, 1);

	strcpy(FTPBUFFER, "PASV\r\n");
	if (!ftp_send(ftp->s, FTPBUFFER, strlen(FTPBUFFER), FTPTIMEOUT)) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Connection timedout");

		return 2;
	}

	size_t msg = LuaAddMessagesToTable(L, ftp->s, ftp->log, FTPTIMEOUT);
	msg = LuaGetLogSize(L, ftp->log);
	int result = LuaGetLastMessage(L, ftp->log, msg);

	if (result != 227) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushfstring(L, "Connection failed: %d", result);

		return 2;
	}

	char ipaddr[255];
	int a, b, c, d;
	int pa, pb;
	char* find = strrchr(FTPBUFFER, '(');
	sscanf(find, "(%d,%d,%d,%d,%d,%d)", &a, &b, &c, &d, &pa, &pb);
	sprintf(ipaddr, "%d.%d.%d.%d", a, b, c, d);
	int port = pa * 256 + pb;

	lua_pop(L, lua_gettop(L));
	lua_pushstring(L, ipaddr);
	lua_pushinteger(L, port);

	if (ftp->passive_ip) {
		free(ftp->passive_ip);
	}

	ftp->passive_ip = (char*)calloc(strlen(ipaddr) + 1, sizeof(char));
	strcpy(ftp->passive_ip, ipaddr);
	ftp->passive_port = port;

	return 2;
}

int LuaCommand(lua_State* L) {

	LuaFTP* ftp = lua_toluaftp(L, 1);
	const char* cmd = luaL_checkstring(L, 2);

	sprintf(FTPBUFFER, "%s\r\n", cmd);
	if (!ftp_send(ftp->s, FTPBUFFER, strlen(FTPBUFFER), FTPTIMEOUT)) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Connection timedout");

		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int LuaConnect(lua_State* L) {

	const char* addr = luaL_checkstring(L, 1);
	int port = luaL_optinteger(L, 2, 21);

	SOCKET s = INVALID_SOCKET;
	char portstr[15];
	struct addrinfo* result = NULL, * ptr = NULL, hints;

	sprintf(portstr, "%d", port);

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int iResult = getaddrinfo(addr, portstr, &hints, &result);

	if (iResult != 0) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushfstring(L, "Unable to resolve address %s:%s", addr, portstr);

		return 2;
	}

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

	u_long flag = 1;
	ioctlsocket(s, FIONBIO, &flag);

	int code = ftp_recv(s, FTPBUFFER, FTPBUFFERSIZE, FTPTIMEOUT);
	int idx = 0;

	if (code == 220) {

		lua_pop(L, lua_gettop(L));
		LuaFTP* ftp = lua_pushluaftp(L);
		ftp->s = s;
		lua_newtable(L);
		ftp->log = luaL_ref(L, LUA_REGISTRYINDEX);

		lua_newtable(L);

		while (code == 220) {

			lua_pushstring(L, FTPBUFFER);
			lua_rawseti(L, -2, ++idx);

			code = ftp_recv(s, FTPBUFFER, FTPBUFFERSIZE, 0);
		}
	}
	else {

		closesocket(s);

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushfstring(L, "Bad server response: %d", code);

		return 2;
	}

	return 2;
}

LuaFTP* lua_pushluaftp(lua_State* L) {

	LuaFTP* ftp = (LuaFTP*)lua_newuserdata(L, sizeof(LuaFTP));

	if (ftp == NULL)
		luaL_error(L, "Unable to push ftp");

	luaL_getmetatable(L, LUAFTP);
	lua_setmetatable(L, -2);

	memset(ftp, 0, sizeof(LuaFTP));

	return ftp;
}

LuaFTP* lua_toluaftp(lua_State* L, int index) {

	LuaFTP* ftp = (LuaFTP*)luaL_checkudata(L, index, LUAFTP);

	if (ftp == NULL)
		luaL_error(L, "parameter is not a %s", LUAFTP);

	return ftp;
}

int luaftp_gc(lua_State* L) {

	LuaFTP* ftp = lua_toluaftp(L, 1);

	if (ftp->s != INVALID_SOCKET) {
		closesocket(ftp->s);
		ftp->s = INVALID_SOCKET;
	}

	if (ftp->passive_ip) {

		free(ftp->passive_ip);
		ftp->passive_ip = NULL;
	}

	if (ftp->log > 0) {
		luaL_unref(L, LUA_REGISTRYINDEX, ftp->log);
		ftp->log = 0;
	}

	return 0;
}

int luaftp_tostring(lua_State* L) {
	char tim[1024];
	sprintf(tim, "FTP: 0x%08X", lua_toluaftp(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}