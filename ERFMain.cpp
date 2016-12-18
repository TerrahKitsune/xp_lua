#include "ERFMain.h"
#include "Erf.h"

static const struct luaL_Reg erffunctions[] = {
	{ "Open", OpenErf },
	{ "GetResource", GetResource },
	{ "GetStrings", GetLocalizedStrings },
	{ "GetKeys", GetKeys },
	//{ "AddFile", AddFile },
	{ NULL, NULL }
};

static const luaL_Reg erfmeta[] = {
	{ "__gc", erf_gc },
	{ "__tostring", erf_tostring },
	{ NULL, NULL }
};

int luaopen_erf(lua_State *L) {

	luaL_newlibtable(L, erffunctions);
	luaL_setfuncs(L, erffunctions, 0);

	luaL_newmetatable(L, LERF);
	luaL_setfuncs(L, erfmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}