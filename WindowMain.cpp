#include "luawindow.h"
#include "WindowMain.h"
#include "customdrawing.h"
#include "customwindow.h"
#include "customcombobox.h"
#include "customtextbox.h"
#include <commctrl.h>

static const struct luaL_Reg windowfunctions[] = {

	{ "GetID", LuaWindowGetId },
	{ "GetInfo", GetWindowInformation},
	{ "GetProcessId", GetWindowProcessId},
	{ "GetText", GetText},
	{ "GetIsVisible", GetIsVisible },
	{ "GetIsEnabled", GetsWindowEnabled },
	{ "GetWindow", GetWindow },
	{ "Open", OpenWindow},
	{ "GetParent", GetWindowParent},
	{ "Destroy", LuaDestroyWindow },
	{ "GetContent", LuaGetContent },
	{ "SetContent", LuaSetContent },

	{ "Create", CreateLuaWindow },
	{ "Redraw", InvalidateWindow},
	{ "Show", ShowCustomWindow },
	{ "Enable", LuaEnableCustomWindow },
	{ "GetThread", GetCustomWindowCoroutine },
	{ "SetDrawFunction", LuaSetDrawFunction },
	{ "CheckHasMessage", LuaCheckHasMessage },
	{ "CreateButton", CreateLuaButton },
	{ "CreateTextBox", CreateTextField },
	{ "CreateComboBox", CreateCustomLuaComboBox },
	{ "AddBoxItem", AddBoxItem},
	{ "RemoveBoxItem", DeleteBoxItem},
	{ "GetBoxItems", GetBoxItems },
	{ "CreateListBox", CreateCustomLuaListbox },
	{ "CreateListView" , CreateCustomLuaListView },

	{ NULL, NULL }
};

static const luaL_Reg windowmeta[] = {
	{ "__gc",  window_gc },
	{ "__tostring",  window_tostring },
	{ NULL, NULL }
};

int luaopen_window(lua_State* L) {

	CoInitialize(NULL);

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