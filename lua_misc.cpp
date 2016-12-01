#include "lua_misc.h"
#include <objbase.h>

int lua_uuid(lua_State*L){

	GUID guid;
	if (CoCreateGuid(&guid) != S_OK){
		lua_pushnil(L);
		return 1;
	}

	char buffer[100];

	sprintf(buffer,"%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
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

int luaopen_misc(lua_State *L){

	lua_pushcfunction(L, lua_uuid);
	lua_setglobal(L, "UUID");

	lua_pushcfunction(L, lua_sleep);
	lua_setglobal(L, "Sleep");

	return 0;
}