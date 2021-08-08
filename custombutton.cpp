#include "customwindow.h"
#include "luawindow.h"
#include "customdrawing.h"
#include "custombutton.h"

void DoCustomButtonEvent(lua_State*L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	if (child->custom->eventRef == LUA_REFNIL) {
		return;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, child->custom->eventRef);
	lua_pushvalue(L, -2);
	lua_pushvalue(L, -5);

	if (lua_pcall(L, 2, 0, NULL)) {

		lua_error(L);
		return;
	}
}

int CreateCustomLuaButton(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!window->custom) {

		lua_pushnil(L);
		return 1;
	}
	else if (lua_type(L, 7) != LUA_TFUNCTION) {
		luaL_error(L, "Missing function parameter");
		return 0;
	}

	LuaCustomWindow* custom = CreateCustomWindowStruct();

	if (!custom) {
		luaL_error(L, "out of memory");
		return 0;
	}

	custom->hmenu = (HMENU)GetLuaChildrenCount(L, window->custom) + 1;

	HWND hwndButton = CreateWindow(
		"BUTTON",
		luaL_checkstring(L, 2),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		(int)luaL_optnumber(L, 3, 0),
		(int)luaL_optnumber(L, 4, 0),
		(int)luaL_optnumber(L, 5, 0),
		(int)luaL_optnumber(L, 6, 0),
		window->handle,
		custom->hmenu,
		(HINSTANCE)GetWindowLongPtr(window->handle, GWLP_HINSTANCE),
		NULL);

	lua_pushvalue(L, 7);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, 1);
	int refParent = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_pop(L, lua_gettop(L));

	LuaWindow* button = lua_pushwindow(L);
	button->handle = hwndButton;
	button->custom = custom;
	button->custom->type = WINDOW_TYPE_BUTTON;
	button->custom->eventRef = ref;
	button->custom->parentRef = refParent;

	AddLuaTableChild(L, window->custom);

	return 1;
}