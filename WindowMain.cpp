#include "luawindow.h"
#include "WindowMain.h"
#include "customdrawing.h"
#include "customwindow.h"
#include "customcombobox.h"
#include "customtextbox.h"
#include "customprogressbar.h"

static const struct luaL_Reg windowfunctions[] = {

	{ "GetID", LuaWindowGetId },
	{ "GetInfo", GetWindowInformation},
	{ "GetProcessId", GetWindowProcessId},
	{ "GetText", GetText},
	{ "GetIsVisible", GetIsVisible },
	{ "GetIsEnabled", GetsWindowEnabled },
	{ "GetWindow", GetWindow },
	{ "Open", OpenWindow },
	{ "GetParent", GetWindowParent },
	{ "Destroy", LuaDestroyWindow },
	{ "GetContent", LuaGetContent },
	{ "SetContent", LuaSetContent },
	{ "Size", GetWindowSize },

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
	{ "GetBoxSelectedIndex", GetSelectedIndex },
	{ "SetBoxSelectedIndex", SetSelectedIndex },
	{ "CreateListBox", CreateCustomLuaListbox },
	{ "CreateListView" , CreateCustomLuaListView },
	{ "Move", MoveCustomWindow },
	{ "SetListViewText", ListviewSetItemText },
	{ "SetListViewColumnWidth", SetViewlistColumnWidth },
	{ "CreateProgressbar", CreateCustomLuaProgressbar },
	{ "StepProgressbar", LuaProgressbarStep },
	{ "GetProgressbarStep", LuaProgressbarGetStep },
	{ "CreateStaticText", CreateStaticTextField },

	{ NULL, NULL }
};

static const luaL_Reg windowmeta[] = {
	{ "__gc",  window_gc },
	{ "__tostring",  window_tostring },
	{ NULL, NULL }
};

int luaopen_window(lua_State* L) {

	CoInitialize(NULL);
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_ANIMATE_CLASS | 
		ICC_BAR_CLASSES | 
		ICC_COOL_CLASSES | 
		ICC_DATE_CLASSES | 
		ICC_INTERNET_CLASSES |
		ICC_LINK_CLASS |
		ICC_LISTVIEW_CLASSES |
		ICC_NATIVEFNTCTL_CLASS |
		ICC_PAGESCROLLER_CLASS |
		ICC_PROGRESS_CLASS |
		ICC_STANDARD_CLASSES |
		ICC_TAB_CLASSES |
		ICC_TREEVIEW_CLASSES |
		ICC_UPDOWN_CLASS |
		ICC_USEREX_CLASSES |
		ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);

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