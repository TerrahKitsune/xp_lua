#include "luaaes.h"
#include "LuaAesMain.h"

static const struct luaL_Reg luaaesfunctions[] = {
	{ "Encrypt", LuaAesEncrypt },
	{ "Decrypt", LuaAesDecrypt },
	{ "Create",  LuaCreateContext },
	{ "SetIV",  LuaSetIV },
	{ NULL, NULL }
};

static const luaL_Reg luaaesmeta[] = {
	{ "__gc",  luaaes_gc },
	{ "__tostring",  luaaes_tostring },
{ NULL, NULL }
};

int luaopen_luaaes(lua_State *L) {

	luaL_newlibtable(L, luaaesfunctions);
	luaL_setfuncs(L, luaaesfunctions, 0);

	luaL_newmetatable(L, LUAES);
	luaL_setfuncs(L, luaaesmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}