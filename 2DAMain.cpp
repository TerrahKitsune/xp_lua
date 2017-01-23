#include "2damain.h"
#include "2da.h"

static const struct luaL_Reg twodafunctions[] = {
	{ "Open", twoda_open },
	{ "Get2DAString", twoda_get2dastring },
	{ "Get2DARow", twoda_get2darow },
	{ "GetInfo", twoda_get2dainfo },
	{ NULL, NULL }
};

static const luaL_Reg twodameta[] = {
	{ "__gc", twoda_gc },
	{ "__tostring", twoda_tostring },
	{ NULL, NULL }
};

int luaopen_twoda(lua_State *L) {

	luaL_newlibtable(L, twodafunctions);
	luaL_setfuncs(L, twodafunctions, 0);

	luaL_newmetatable(L, TWODA);
	luaL_setfuncs(L, twodameta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}