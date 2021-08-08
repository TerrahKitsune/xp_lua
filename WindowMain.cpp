#include "luawindow.h"
#include "WindowMain.h"
#include "customdrawing.h"

static const struct luaL_Reg windowfunctions[] = {

	{ "GetID", LuaWindowGetId },
	{ "GetInfo", GetWindowInformation},
	{ "GetProcessId", GetWindowProcessId},
	{ "GetText", GetText},
	{ "GetIsVisible", GetIsVisible },
	{ "GetWindow", GetWindow },
	{ "Open", OpenWindow},
	{ "GetParent", GetWindowParent},
	{ "Destroy", LuaDestroyWindow },

	{ "Create", CreateLuaWindow },
	{ "Show", ShowCustomWindow },
	{ "GetThread", GetCustomWindowCoroutine },
	{ "SetDrawFunction", LuaSetDrawFunction },
	{ "CheckHasMessage", LuaCheckHasMessage },
	{ "CreateButton", CreateLuaButton },

	{ NULL, NULL }
};

static const luaL_Reg windowmeta[] = {
	{ "__gc",  window_gc },
	{ "__tostring",  window_tostring },
	{ NULL, NULL }
};

int luaopen_window(lua_State* L) {

	luaL_newlibtable(L, windowfunctions);
	luaL_setfuncs(L, windowfunctions, 0);

	luaL_newmetatable(L, LUAWINDOW);
	luaL_setfuncs(L, windowmeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}