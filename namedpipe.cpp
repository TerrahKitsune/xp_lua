#include "namedpipe.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 

int ReadByte(lua_State *L) {

	LuaNamedPipe * pipe = lua_tonamedpipe(L, 1);

	DWORD read = 0;
	BOOL success = PeekNamedPipe(pipe->Pipe, NULL, 0, NULL, &read, NULL);

	if (success && read > 0) {

		unsigned char data;

		success = ReadFile(pipe->Pipe, &data, 1, &read, NULL);

		if (success && read > 0) {

			lua_pop(L, lua_gettop(L));
			lua_pushinteger(L, data);
			return 1;
		}
	}

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, -1);
	return 1;
}

int ReadPipe(lua_State *L) {

	LuaNamedPipe * pipe = lua_tonamedpipe(L, 1);
	int buffersize = max(luaL_optinteger(L, 2, 1024),1024);

	char * buf = (char*)malloc(buffersize);

	if (!buf) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	DWORD read = 1;
	BOOL success = PeekNamedPipe(pipe->Pipe, buf, 1, NULL, &read, NULL);

	if (success && read > 0) {
		success = ReadFile(pipe->Pipe, buf, buffersize - 1, &read, NULL);
		buf[read] = '\0';
		lua_pop(L, lua_gettop(L));
		lua_pushlstring(L, buf, read);
		free(buf);
		return 1;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushnil(L);
	free(buf);
	return 1;
}

int WriteByte(lua_State *L) {

	LuaNamedPipe * pipe = lua_tonamedpipe(L, 1);
	unsigned char byte = luaL_checkinteger(L, 2);
	DWORD written;

	if (WriteFile(pipe->Pipe, &byte, 1, &written, NULL)) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, written>0);
		return 1;
	}
	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, false);
	return 1;
}

int WritePipe(lua_State *L) {

	LuaNamedPipe * pipe = lua_tonamedpipe(L, 1);
	size_t len;
	const char * data = luaL_checklstring(L, 2, &len);
	DWORD written;

	if (WriteFile(pipe->Pipe, data, len, &written, NULL)) {
		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, written);
		return 1;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushnil(L);
	lua_pushinteger(L, GetLastError());
	return 2;
}

int OpenNamedPipe(lua_State *L) {

	size_t len;
	const char * pipename = luaL_checklstring(L, 1, &len);
	DWORD access = 0;

	if (len >= MAX_PATH - 9) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		//Invalid name
		lua_pushinteger(L, 0x7B);
		return 2;
	}

	if (lua_gettop(L) >= 2 && lua_toboolean(L, 2)) {
		access |= GENERIC_READ;
	}

	if (lua_gettop(L) >= 3 && lua_toboolean(L, 3)) {
		access |= GENERIC_WRITE;
	}

	if (access == 0) {
		access = GENERIC_READ | GENERIC_WRITE;
	}

	char realpipename[MAX_PATH] = "\\\\.\\pipe\\";
	strcat(realpipename, pipename);

	HANDLE hpipe = CreateFile(realpipename, access, 0, NULL, OPEN_EXISTING, 0, NULL);

	lua_pop(L, lua_gettop(L));

	if (hpipe == INVALID_HANDLE_VALUE) {
		lua_pushnil(L);
		lua_pushinteger(L, GetLastError());
		return 2;
	}

	LuaNamedPipe * pipe = lua_pushnamedpipe(L);

	pipe->Pipe = hpipe;

	return 1;
}

int CreateNamedPipe(lua_State *L) {

	size_t len;
	const char * pipename = luaL_checklstring(L, 1, &len);
	int maxinstances = luaL_optinteger(L, 2, 1);
	int buffersize = luaL_optinteger(L, 3, 1024);
	int timeout = luaL_optinteger(L, 4, 1000);
	DWORD access = 0;

	if (len >= MAX_PATH - 9) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		//Invalid name
		lua_pushinteger(L, 0x7B);
		return 2;
	}

	if (lua_gettop(L) >= 5 && lua_toboolean(L, 5)) {
		access |= PIPE_ACCESS_INBOUND;
	}

	if (lua_gettop(L) >= 6 && lua_toboolean(L, 6)) {
		access |= PIPE_ACCESS_OUTBOUND;
	}

	if (access == 0) {
		access = PIPE_ACCESS_DUPLEX;
	}

	buffersize = max(maxinstances, 1024);
	timeout = max(timeout, 1);

	char realpipename[MAX_PATH] = "\\\\.\\pipe\\";
	strcat(realpipename, pipename);

	HANDLE hpipe = CreateNamedPipe(realpipename, access, PIPE_NOWAIT, max(maxinstances, 1), buffersize, buffersize, timeout, NULL);

	lua_pop(L, lua_gettop(L));

	if (hpipe == INVALID_HANDLE_VALUE) {
		lua_pushnil(L);
		lua_pushinteger(L, GetLastError());
		return 2;
	}

	LuaNamedPipe * pipe = lua_pushnamedpipe(L);

	pipe->Pipe = hpipe;

	return 1;
}

LuaNamedPipe * lua_pushnamedpipe(lua_State *L) {
	LuaNamedPipe * namedpipe = (LuaNamedPipe*)lua_newuserdata(L, sizeof(LuaNamedPipe));
	if (namedpipe == NULL)
		luaL_error(L, "Unable to push namedpipe");
	luaL_getmetatable(L, NAMEDPIPE);
	lua_setmetatable(L, -2);
	memset(namedpipe, 0, sizeof(LuaNamedPipe));
	namedpipe->Pipe = INVALID_HANDLE_VALUE;
	return namedpipe;
}

LuaNamedPipe * lua_tonamedpipe(lua_State *L, int index) {
	LuaNamedPipe * namedpipe = (LuaNamedPipe*)luaL_checkudata(L, index, NAMEDPIPE);
	if (namedpipe == NULL)
		luaL_error(L, "parameter is not a %s", NAMEDPIPE);
	return namedpipe;
}

int namedpipe_gc(lua_State *L) {

	LuaNamedPipe * pipe = lua_tonamedpipe(L, 1);

	if (pipe->Pipe != INVALID_HANDLE_VALUE) {
		CloseHandle(pipe->Pipe);
		pipe->Pipe = INVALID_HANDLE_VALUE;
	}

	return 0;
}

int namedpipe_tostring(lua_State *L) {
	char tim[100];
	sprintf(tim, "Namedpipe: 0x%08X", lua_tonamedpipe(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}