#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
static const char* LUAMUTEX = "LUAMUTEX";

typedef struct LuaMutex {

	HANDLE mutex;
	char mutexname[MAX_PATH];
	bool istaken;

} LuaMutex;


LuaMutex* lua_pushmutex(lua_State* L);
LuaMutex* lua_tomutex(lua_State* L, int index);

int LuaCreateMutex(lua_State* L);
int LuaLockMutex(lua_State* L);
int LuaUnlockMutex(lua_State* L);
int LuaGetMutexInfo(lua_State* L);

int mutex_gc(lua_State* L);
int mutex_tostring(lua_State* L);