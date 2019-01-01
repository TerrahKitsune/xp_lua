#include "lua_misc.h"
#include <objbase.h>
#include <time.h>

static int env_table = -1;
static int env_original = -1;

int lua_uuid(lua_State*L){

	GUID guid;
	if (CoCreateGuid(&guid) != S_OK){
		lua_pushnil(L);
		return 1;
	}

	char buffer[100];

	sprintf(buffer,"%08lx-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	lua_pushstring(L, buffer);

	return 1;
}

int lua_sleep(lua_State*L){

	int zzz = luaL_optinteger(L, 1, 1);

	if (zzz <= 0)
		zzz = 1;
	else if (zzz > 1000)
		zzz = 1000;

	Sleep(zzz);
	lua_pop(L,1);
	return 0;
}

static int GetLastErrorAsMessage(lua_State *L)
{
	DWORD lasterror = luaL_optinteger(L, 1, GetLastError());
	char err[1024];

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lasterror,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 1024, NULL);

	lua_pop(L, lua_gettop(L));
	lua_pushstring(L, err);
	lua_pushinteger(L, lasterror);

	return 2;
}

int Time(lua_State *L) {

	lua_pushinteger(L, time(NULL));
	return 1;
}

int NewEnvironment(lua_State *L) {

	if (!lua_isstring(L, 1) || lua_gettop(L)!=1) {
		luaL_error(L, "Invalid parameters");
		return 0;
	}

	lua_newtable(L);

	lua_rawgeti(L, LUA_REGISTRYINDEX, env_table);
	
	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	lua_settable(L, -3);

	lua_pushvalue(L, 2);
	lua_copy(L, 2, 1);
	lua_pop(L, 3);

	return 1;
}

int GetEnvironment(lua_State *L) {

	if (!lua_isstring(L, 1) || lua_gettop(L) != 1) {
		luaL_error(L, "Invalid parameters");
		return 0;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, env_table);

	lua_pushvalue(L, 1);
	lua_gettable(L, -2);

	lua_copy(L, 3, 1);

	lua_pop(L, 2);

	return 1;
}

int GetCreateEnvironment(lua_State *L) {

	const char * name = luaL_checkstring(L, 1);

	GetEnvironment(L);
	if (lua_istable(L, 1)) {
		return 1;
	}
	else {
		lua_pop(L, lua_gettop(L));
		lua_pushstring(L, name);
		return NewEnvironment(L);
	}
}

int luaopen_misc(lua_State *L){

	lua_newtable(L);
	env_table = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_pushcfunction(L, GetLastErrorAsMessage);
	lua_setglobal(L, "GetLastError");

	lua_pushcfunction(L, lua_uuid);
	lua_setglobal(L, "UUID");

	lua_pushcfunction(L, lua_sleep);
	lua_setglobal(L, "Sleep");

	lua_pushcfunction(L, Time);
	lua_setglobal(L, "Time");

	lua_newtable(L);

	lua_pushstring(L, "Create");
	lua_pushcfunction(L, NewEnvironment);
	lua_settable(L, -3);

	lua_pushstring(L, "Get");
	lua_pushcfunction(L, GetEnvironment);
	lua_settable(L, -3);

	lua_pushstring(L, "GetOrCreate");
	lua_pushcfunction(L, GetCreateEnvironment);
	lua_settable(L, -3);

	lua_setglobal(L, "Env");

	return 0;
}