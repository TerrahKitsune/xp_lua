#include "NWN2LuaLib.h"
#include <string.h>
#include "AssemblyHelper.h"

void register_c_function(lua_State *L, const char * tableName, const char * funcName, lua_CFunction funcPointer)
{
	lua_getglobal(L, tableName);
	if (!lua_istable(L, -1))
	{
		lua_createtable(L, 0, 1);
		lua_setglobal(L, tableName);
		lua_getglobal(L, tableName);
	}

	lua_pushstring(L, funcName);
	lua_pushcfunction(L, funcPointer);
	lua_settable(L, -3);

	lua_pop(L, 1);
}

void lua_nwn2_openlib(lua_State * L){

	register_c_function(L, "debug", "SignatureScan", debug_SignatureScan);
}

int debug_SignatureScan(lua_State * L){

	const char * signature = luaL_checkstring(L,1);
	unsigned long result = 0;

	AssemblyHelper * ah = new AssemblyHelper();
	result = ah->FindFunctionBySignature(signature);
	delete ah;

	lua_pop(L,1);

	lua_pushfstring(L,"0x%08X",result);

	return 1;
}
