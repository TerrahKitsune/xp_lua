#include "LuaFTPChannel.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 

int LuaFtpChannelSend(lua_State* L) {

	LuaFTPChannel* ftp = lua_toluaftpchannel(L, 1);
	size_t len;
	const char * data = lua_tolstring(L, 2, &len);

	if (ftp->s == INVALID_SOCKET || !data || len <= 0) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		return 1;
	}

	size_t sent = 0;
	int result = 0;

	do {

		result = send(ftp->s, data, len - sent, 0);

		if (result == 0) {

			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			return 1;
		}
		else if (result < 0) {

			if (WSAGetLastError() != WSAEWOULDBLOCK) {

				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				return 1;
			}
			else {

				Sleep(1);
			}
		}
		else {
			sent += result;
		}

	} while (len - sent > 0);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int LuaFtpChannelRecv(lua_State* L) {

	LuaFTPChannel* ftp = lua_toluaftpchannel(L, 1);
	size_t buffersize = luaL_optinteger(L, 2, 4096);

	if (ftp->s == INVALID_SOCKET) {

		lua_pop(L, lua_gettop(L));
		lua_pushstring(L, "");
		return 1;
	}
	else if (!ftp->buffer || ftp->buffersize != buffersize) {

		if (ftp->buffer) {
			gff_free(ftp->buffer);
		}

		ftp->buffer = (char*)gff_malloc(buffersize);
		ftp->buffersize = buffersize;
	}

	if (!ftp->buffer) {
		luaL_error(L, "Unable to allocate recv buffer");
		return 0;
	}

	int result = recv(ftp->s, ftp->buffer, ftp->buffersize, 0);

	lua_pop(L, lua_gettop(L));

	if (result == 0) {

		lua_pushboolean(L, false);
		lua_pushstring(L, "");
	}
	else if (result < 0) {

		lua_pushboolean(L, WSAGetLastError() == WSAEWOULDBLOCK);
		lua_pushstring(L, "");
	}
	else {

		lua_pushboolean(L, true);
		lua_pushlstring(L, ftp->buffer, result);
	}

	return 2;
}

int LuaFtpChannelGetConnectionStatus(lua_State* L) {

	LuaFTPChannel* ftp = lua_toluaftpchannel(L, 1);

	bool isConnected = true;
	char peek[2];

	int result = ftp->s == INVALID_SOCKET ? 0 : recv(ftp->s, peek, 1, MSG_PEEK);

	if (result == 0) {
		isConnected = false;
	}
	else if (result < 0) {
		isConnected = WSAGetLastError() == WSAEWOULDBLOCK;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, isConnected);
	lua_pushboolean(L, result > 0);

	return 2;
}

LuaFTPChannel* lua_pushluaftpchannel(lua_State* L) {

	LuaFTPChannel* ftp = (LuaFTPChannel*)lua_newuserdata(L, sizeof(LuaFTPChannel));

	if (ftp == NULL)
		luaL_error(L, "Unable to push ftp data channel");

	luaL_getmetatable(L, FTPCHANNEL);
	lua_setmetatable(L, -2);

	memset(ftp, 0, sizeof(LuaFTPChannel));

	return ftp;
}

LuaFTPChannel* lua_toluaftpchannel(lua_State* L, int index) {

	LuaFTPChannel* ftp = (LuaFTPChannel*)luaL_checkudata(L, index, FTPCHANNEL);

	if (ftp == NULL)
		luaL_error(L, "parameter is not a %s", FTPCHANNEL);

	return ftp;
}

int luaftpchannel_gc(lua_State* L) {

	LuaFTPChannel* ftp = lua_toluaftpchannel(L, 1);

	if (ftp->s != INVALID_SOCKET) {
		closesocket(ftp->s);
		ftp->s = INVALID_SOCKET;
	}

	if (ftp->ip) {
		gff_free(ftp->ip);
		ftp->ip = NULL;
		ftp->port = 0;
	}

	if (ftp->buffer) {
		gff_free(ftp->buffer);
		ftp->buffer = NULL;
		ftp->buffersize = 0;
	}

	return 0;
}

int luaftpchannel_tostring(lua_State* L) {

	LuaFTPChannel* ftp = lua_toluaftpchannel(L, 1);

	char tim[1024];
	sprintf(tim, "FTPCHANNEL: 0x%08X %s:%d", ftp, ftp->ip, ftp->port);
	lua_pushfstring(L, tim);
	return 1;
}