#include "lua_misc.h"
#include <objbase.h>
#include <time.h>

#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#pragma comment (lib , "winmm.lib")

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

int lua_sleep(lua_State* L) {

	int zzz = (int)luaL_optinteger(L, 1, 1);

	if (zzz <= 0)
		zzz = 1;
	else if (zzz > 1000)
		zzz = 1000;

	Sleep(zzz);
	lua_pop(L, 1);
	return 0;
}

static int GetLastErrorAsMessage(lua_State* L)
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

int Time(lua_State* L) {

	lua_pushinteger(L, time(NULL));
	return 1;
}

int NewEnvironment(lua_State* L) {

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

int GetEnvironment(lua_State* L) {

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

int GetCreateEnvironment(lua_State* L) {

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

int GetAllEnvironment(lua_State* L) {

	lua_rawgeti(L, LUA_REGISTRYINDEX, env_table);

	return 1;
}

int GetStringEqual(lua_State* L) {

	size_t len1;
	size_t len2;
	const char* str1 = lua_tolstring(L, 1, &len1);
	const char* str2 = lua_tolstring(L, 2, &len2);

	lua_pop(L, lua_gettop(L));

	if (!str1 || !str2) {
		lua_pushboolean(L, str1 == str2);
	}
	else if (str1 == str2 && len1 == len2) {
		lua_pushboolean(L, true);
	}
	else if (len1 == len2) {

		for (size_t i = 0; i < len1; i++)
		{
			if (tolower(str1[i]) != tolower(str2[i])) {
				lua_pushboolean(L, false);
				return 1;
			}
		}

		lua_pushboolean(L, true);
	}
	else {
		lua_pushboolean(L, false);
	}

	return 1;
}

int TableFirst(lua_State* L) {

	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	if (lua_gettop(L) > 2) {
		lua_pop(L, lua_gettop(L) - 2);
	}

	lua_pushvalue(L, 1);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		lua_pushvalue(L, 2);
		lua_pushvalue(L, 4);
		lua_pushvalue(L, 5);

		if (lua_pcall(L, 2, 1, 0) != 0) {
			luaL_error(L, lua_tostring(L, -1));
			return 0;
		}

		if (!lua_isnil(L, -1)) {
			lua_copy(L, -1, 1);
			lua_pop(L, lua_gettop(L) - 1);
			return 1;
		}

		lua_pop(L, 2);
	}

	lua_pop(L, lua_gettop(L));
	lua_pushnil(L);
	return 1;
}

int TableSelect(lua_State* L) {

	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	if (lua_gettop(L) > 2) {
		lua_pop(L, lua_gettop(L) - 2);
	}

	int n = 0;
	lua_createtable(L, 0, 0);
	lua_pushvalue(L, 1);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		lua_pushvalue(L, 2);
		lua_pushvalue(L, 5);
		lua_pushvalue(L, 6);

		if (lua_pcall(L, 2, 1, 0) != 0) {
			luaL_error(L, lua_tostring(L, -1));
			return 0;
		}

		if (!lua_isnil(L, -1)) {

			lua_rawseti(L, 3, ++n);
			lua_pop(L, 1);
		}
		else {
			lua_pop(L, 2);
		}
	}

	lua_copy(L, 3, 1);
	lua_pop(L, lua_gettop(L) - 1);

	return 1;
}

DWORD crc32(byte* data, int size, DWORD crc)
{
	DWORD r = crc;
	byte* end = data + size;
	DWORD t;

	while (data < end)
	{
		r ^= *data++;

		for (int i = 0; i < 8; i++)
		{
			t = ~((r & 1) - 1); 
			r = (r >> 1) ^ (0xEDB88320 & t);
		}
	}

	return ~r;
}

int CRC32(lua_State* L) {

	size_t size;
	const char* data = lua_tolstring(L, 1, &size);
	DWORD crc = 0xFFFFFFFF;

	if (lua_isnumber(L, 2)) {
		crc = ~(DWORD)lua_tonumber(L, 2);
	}

	crc = crc32((BYTE*)data, size, crc);

	lua_pop(L, lua_gettop(L));

	lua_pushinteger(L, crc);

	return 1;
}

int luabeep(lua_State* L) {

	DWORD freq = luaL_checkinteger(L, 1);
	DWORD dur = luaL_checkinteger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, Beep(freq, dur));

	return 1;
}

int luasound(lua_State* L) {

	const char* sound = NULL;

	if (!lua_isnoneornil(L, 1)) {
		sound = lua_tostring(L, 1);
	}

	DWORD Flags = SND_FILENAME | SND_NODEFAULT;

	if (lua_toboolean(L, 2)) {
		Flags |= SND_ASYNC;
	}
	else {
		Flags |= SND_SYNC;
	}

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, PlaySound(sound, NULL, Flags));

	return 1;
}

int luaopen_misc(lua_State* L) {

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

	lua_newtable(L);

	lua_pushstring(L, "Play");
	lua_pushcfunction(L, luasound);
	lua_settable(L, -3);

	lua_pushstring(L, "Beep");
	lua_pushcfunction(L, luabeep);
	lua_settable(L, -3);

	lua_setglobal(L, "Sound");

	lua_getglobal(L, "string");
	lua_pushstring(L, "equal");
	lua_pushcfunction(L, GetStringEqual);
	lua_settable(L, -3);
	lua_pop(L, 1);

	lua_getglobal(L, "table");
	lua_pushstring(L, "first");
	lua_pushcfunction(L, TableFirst);
	lua_settable(L, -3);
	lua_pushstring(L, "select");
	lua_pushcfunction(L, TableSelect);
	lua_settable(L, -3);
	lua_pop(L, 1);

	lua_pushcfunction(L, GetLastErrorAsMessage);
	lua_setglobal(L, "GetLastError");

	lua_pushcfunction(L, lua_uuid);
	lua_setglobal(L, "UUID");

	lua_pushcfunction(L, lua_sleep);
	lua_setglobal(L, "Sleep");

	lua_pushcfunction(L, Time);
	lua_setglobal(L, "Time");

	lua_pushcfunction(L, CRC32);
	lua_setglobal(L, "CRC32");

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