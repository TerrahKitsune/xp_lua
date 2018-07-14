#include "HttpMain.h"
#include "Http.h"

static const struct luaL_Reg httpfunctions[] = {
	{ "Start", Start },
	{ "SetTimeout", SetHttpTimeout },
	{ "GetResult", GetResult },
	{ "GetStatus", GetStatus },
	{ NULL, NULL }
};

static const luaL_Reg httpmeta[] = {

	{ "__gc", luahttp_gc },
	{ "__tostring", luahttp_tostring },
	{ NULL, NULL }
};

int luaopen_http(lua_State *L) {

	luaL_newlibtable(L, httpfunctions);
	luaL_setfuncs(L, httpfunctions, 0);

	luaL_newmetatable(L, LUAHTTP);
	luaL_setfuncs(L, httpmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}