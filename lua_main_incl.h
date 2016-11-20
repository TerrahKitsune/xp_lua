#pragma once
#include "xp_lua_incl.h"

static void DumpStack(lua_State *L, bool untilnil = false){

	FILE * file = fopen("STACK.txt", "w");
	if (!file)
		return;

	for (int n = 1; n <= lua_gettop(L); n++){

		if (untilnil && lua_isnil(L, n))
			return;

		fprintf(file, "%d: ", n);

		switch (lua_type(L, n)){
		case LUA_TNIL:
			fprintf(file, "NIL");
			break;
		case LUA_TNUMBER:
			fprintf(file, "NUMBER %f", lua_tonumber(L, n));
			break;
		case LUA_TBOOLEAN:
			fprintf(file, "BOOLEAN %s", lua_toboolean(L, n) == 0 ? "FALSE" : "TRUE");
			break;
		case LUA_TSTRING:
			fprintf(file, "STRING %s", lua_tostring(L, n));
			break;
		case LUA_TTABLE:
			fprintf(file, "TABLE 0x%08X", lua_topointer(L, n));
			break;
		case LUA_TFUNCTION:
			fprintf(file, "FUNCTION 0x%08X", lua_topointer(L, n));
			break;
		case LUA_TUSERDATA:
			fprintf(file, "USERDATA 0x%08X", lua_topointer(L, n));
			break;
		case LUA_TTHREAD:
			fprintf(file, "THREAD 0x%08X", lua_topointer(L, n));
			break;
		case LUA_TLIGHTUSERDATA:
			fprintf(file, "LIGHTUSERDATA 0x%08X", lua_topointer(L, n));
			break;
		}

		fprintf(file, "\n");
	}

	fflush(file);
	fclose(file);
}