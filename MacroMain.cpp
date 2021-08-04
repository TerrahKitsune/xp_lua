#include "macro.h"
#include "MacroMain.h"

static const struct luaL_Reg macrofunctions[] = {

	{ "GetInputs", GetInputs },
	{ "Send", SendMacro},
	{ "Create", CreateMacro},
	{ "ScreenToMousePoint", ScreenToMouse },
	{ NULL, NULL }
};

static const luaL_Reg macroemeta[] = {
	{ "__gc",  macro_gc },
	{ "__tostring",  macro_tostring },
{ NULL, NULL }
};

int luaopen_macro(lua_State* L) {

	luaL_newlibtable(L, macrofunctions);
	luaL_setfuncs(L, macrofunctions, 0);

	luaL_newmetatable(L, MACRO);
	luaL_setfuncs(L, macroemeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}