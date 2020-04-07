#define _WIN32_WINNT 0x0500
#include "LuaMutex.h"
#include <string.h>
#include <stdlib.h> 
#include <windows.h> 
#include <sddl.h>

int LuaCreateMutex(lua_State* L) {

	const char* name = luaL_checkstring(L, 1);
	size_t len = strlen(name);
	char mutexname[MAX_PATH];

	strcpy(mutexname, "Global\\");

	if (len <= 0 || len + strlen(mutexname) >= (MAX_PATH - 1)) {
		luaL_error(L, "Mutex name is invalid");
		return 0;
	}

	for (size_t n = 0; n < len; n++) {
		if (name[n] == '\\') {
			luaL_error(L, "Mutex name is invalid");
			return 0;
		}
	}

	strcat(mutexname, name);

	HANDLE mutex = CreateMutex(NULL, false, mutexname);

	lua_pop(L, lua_gettop(L));

	if (mutex == NULL) {

		lua_pushnil(L);
		lua_pushinteger(L, GetLastError());

		return 2;
	}

	LuaMutex* luamutex = lua_pushmutex(L);

	memcpy(luamutex->mutexname, mutexname, MAX_PATH);
	luamutex->mutex = mutex;

	return 1;
}

int LuaLockMutex(lua_State* L) {

	LuaMutex* mutex = lua_tomutex(L, 1);
	lua_Integer timeout = luaL_optinteger(L, 2, -1);

	if (mutex->istaken) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, mutex->istaken);

		return 1;
	}

	DWORD wait = INFINITE;

	if (timeout > 0) {
		wait = (DWORD)timeout;
	}

	DWORD result = WaitForSingleObject(mutex->mutex, timeout);

	mutex->istaken = result == WAIT_OBJECT_0;

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, mutex->istaken);

	return 1;
}

int LuaUnlockMutex(lua_State* L) {

	LuaMutex* mutex = lua_tomutex(L, 1);

	if (!mutex->istaken) {

		lua_pop(L, lua_gettop(L));
		return 0;
	}

	DWORD result = ReleaseMutex(mutex->mutex);

	mutex->istaken = false;

	lua_pop(L, lua_gettop(L));

	return 0;
}

int LuaGetMutexInfo(lua_State* L) {

	LuaMutex* mutex = lua_tomutex(L, 1);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, mutex->istaken);
	lua_pushstring(L, mutex->mutexname);
	lua_pushinteger(L, (DWORD)mutex->mutex);

	return 3;
}

LuaMutex* lua_pushmutex(lua_State* L) {

	LuaMutex* mutex = (LuaMutex*)lua_newuserdata(L, sizeof(LuaMutex));
	if (mutex == NULL)
		luaL_error(L, "Unable to push mutex");
	luaL_getmetatable(L, LUAMUTEX);
	lua_setmetatable(L, -2);

	memset(mutex, 0, sizeof(LuaMutex));

	return mutex;
}

LuaMutex* lua_tomutex(lua_State* L, int index) {
	LuaMutex* mutex = (LuaMutex*)luaL_checkudata(L, index, LUAMUTEX);
	if (mutex == NULL)
		luaL_error(L, "parameter is not a %s", LUAMUTEX);
	return mutex;
}

int mutex_gc(lua_State* L) {

	LuaMutex* mutex = lua_tomutex(L, 1);

	if (mutex->mutex) {

		if (mutex->istaken) {
			ReleaseMutex(mutex->mutex);
		}

		CloseHandle(mutex->mutex);
	}

	memset(mutex, 0, sizeof(LuaMutex));

	return 0;
}

int mutex_tostring(lua_State* L) {

	LuaMutex* mutex = lua_tomutex(L, 1);

	char tim[1024];
	sprintf(tim, "Mutex: 0x%08X %s", mutex, mutex->mutexname);
	lua_pushfstring(L, tim);
	return 1;
}