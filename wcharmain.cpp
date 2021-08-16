#include "luawchar.h"
#include "wcharmain.h"

static const struct luaL_Reg wcharfunctions[] = {
	{ "FromAnsi",  FromAnsi },
	{ "ToAnsi", ToAnsi },
	{ "ToWide", ToWide},
	{ NULL, NULL }
};

static const luaL_Reg wcharmeta[] = {
	{ "__concat", wchar_concat },
	{ "__gc",  wchar_gc },
	{ "__tostring",  wchar_tostring },
{ NULL, NULL }
};

int luaopen_wchar(lua_State* L) {

	luaL_newlibtable(L, wcharfunctions);
	luaL_setfuncs(L, wcharfunctions, 0);

	luaL_newmetatable(L, LUAWCHAR);
	luaL_setfuncs(L, wcharmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}