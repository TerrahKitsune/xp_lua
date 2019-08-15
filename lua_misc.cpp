#include "lua_misc.h"
#include <objbase.h>
#include <time.h>

static int env_table = -1;
static int env_original = -1;

int lua_uuid(lua_State* L) {

	GUID guid;
	if (CoCreateGuid(&guid) != S_OK) {
		lua_pushnil(L);
		return 1;
	}

	char buffer[37];

	sprintf(buffer, "%08lx-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	lua_pushstring(L, buffer);

	unsigned long data1 = _byteswap_ulong(guid.Data1);
	unsigned short data2 = _byteswap_ushort(guid.Data2);
	unsigned short data3 = _byteswap_ushort(guid.Data3);

	memcpy(&buffer[0], &data1, sizeof(data1));
	memcpy(&buffer[sizeof(data1)], &data2, sizeof(data2));
	memcpy(&buffer[sizeof(data1) + sizeof(data2)], &data3, sizeof(data3));
	memcpy(&buffer[sizeof(data1) + sizeof(data2) + sizeof(data3)], guid.Data4, sizeof(guid.Data4));
	
	lua_pushlstring(L, buffer, sizeof(data1) + sizeof(data2) + sizeof(data3) + sizeof(guid.Data4));

	return 2;
}

int lua_sleep(lua_State * L) {

	int zzz = (int)luaL_optinteger(L, 1, 1);

	if (zzz <= 0)
		zzz = 1;
	else if (zzz > 1000)
		zzz = 1000;

	Sleep(zzz);
	lua_pop(L, 1);
	return 0;
}

static int GetLastErrorAsMessage(lua_State * L)
{
	DWORD lasterror = (DWORD)luaL_optinteger(L, 1, GetLastError());
	char err[1024];

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lasterror,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 1024, NULL);

	lua_pop(L, lua_gettop(L));
	lua_pushstring(L, err);
	lua_pushinteger(L, lasterror);

	return 2;
}

int Time(lua_State * L) {

	lua_pushinteger(L, time(NULL));
	return 1;
}

int NewEnvironment(lua_State * L) {

	if (!lua_isstring(L, 1) || lua_gettop(L) != 1) {
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

int GetEnvironment(lua_State * L) {

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

int GetCreateEnvironment(lua_State * L) {

	const char* name = luaL_checkstring(L, 1);

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

int GetAllEnvironment(lua_State * L) {

	lua_rawgeti(L, LUA_REGISTRYINDEX, env_table);

	return 1;
}

int luaopen_misc(lua_State * L) {

	lua_newtable(L);
	env_table = luaL_ref(L, LUA_REGISTRYINDEX);

	char esc[2] = { 0,0 };

	lua_createtable(L, 0, 3);

	lua_pushstring(L, "NUL");
	lua_pushlstring(L, esc, 1);
	lua_settable(L, -3);

	for (esc[0] = 1; esc[0] < 32; esc[0]++) {

		switch (esc[0]) {

		case 1:
			lua_pushstring(L, "SOH");
			break;
		case 2:
			lua_pushstring(L, "STX");
			break;
		case 3:
			lua_pushstring(L, "ETX");
			break;
		case 4:
			lua_pushstring(L, "EOT");
			break;
		case 5:
			lua_pushstring(L, "ENQ");
			break;
		case 6:
			lua_pushstring(L, "ACK");
			break;
		case 7:
			lua_pushstring(L, "BEL");
			break;
		case 8:
			lua_pushstring(L, "BS");
			break;
		case 9:
			lua_pushstring(L, "TAB");
			break;
		case 10:
			lua_pushstring(L, "LF");
			break;
		case 11:
			lua_pushstring(L, "VT");
			break;
		case 12:
			lua_pushstring(L, "FF");
			break;
		case 13:
			lua_pushstring(L, "CR");
			break;
		case 14:
			lua_pushstring(L, "SO");
			break;
		case 15:
			lua_pushstring(L, "SI");
			break;
		case 16:
			lua_pushstring(L, "DLE");
			break;
		case 17:
			lua_pushstring(L, "DC1");
			break;
		case 18:
			lua_pushstring(L, "DC2");
			break;
		case 19:
			lua_pushstring(L, "DC3");
			break;
		case 20:
			lua_pushstring(L, "DC4");
			break;
		case 21:
			lua_pushstring(L, "NAK");
			break;
		case 22:
			lua_pushstring(L, "SYN");
			break;
		case 23:
			lua_pushstring(L, "ETB");
			break;
		case 24:
			lua_pushstring(L, "CAN");
			break;
		case 25:
			lua_pushstring(L, "EM");
			break;
		case 26:
			lua_pushstring(L, "SUB");
			break;
		case 27:
			lua_pushstring(L, "ESC");
			break;
		case 28:
			lua_pushstring(L, "FS");
			break;
		case 29:
			lua_pushstring(L, "GS");
			break;
		case 30:
			lua_pushstring(L, "RS");
			break;
		case 31:
			lua_pushstring(L, "US");
			break;
		case 32:
			lua_pushstring(L, "Space");
			break;
		}

		lua_pushstring(L, esc);
		lua_settable(L, -3);
	}


	lua_setglobal(L, "c");

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

	lua_pushstring(L, "Meta");
	lua_pushcfunction(L, GetAllEnvironment);
	lua_settable(L, -3);

	lua_setglobal(L, "Env");

	return 0;
}