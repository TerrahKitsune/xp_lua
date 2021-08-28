#include "customwindow.h"
#include "luawindow.h"
#include "customdrawing.h"
#include "customprogressbar.h"
#include "luawchar.h"
#include <CommCtrl.h>

bool IsProgressbar(LuaWindow* window) {
	
	return window && window->custom && window->custom->type == WINDOW_TYPE_PROGRESSBAR;
}

int LuaProgressbarGetStep(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!IsProgressbar(window)) {

		luaL_error(L, "Window is not a progressbar");
		return 0;
	}

	lua_pushinteger(L, SendMessageW(window->handle, PBM_GETPOS, 0, 0));

	return 1;
}

int LuaProgressbarStep(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!IsProgressbar(window)) {

		luaL_error(L, "Window is not a progressbar");
		return 0;
	}

	if (lua_type(L, 2) == LUA_TNUMBER) {
		SendMessageW(window->handle, PBM_DELTAPOS, (WPARAM)lua_tointeger(L, 2), 0);
	}
	else {
		SendMessageW(window->handle, PBM_STEPIT, 0, 0);
	}

	return 0;
}

int CreateCustomLuaProgressbar(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (!window->custom) {

		lua_pushnil(L);
		return 1;
	}

	LuaCustomWindow* custom = CreateCustomWindowStruct();

	if (!custom) {
		luaL_error(L, "out of memory");
		return 0;
	}

	custom->hmenu = (HMENU)(++window->custom->nextId);

	HWND hwnd = CreateWindowExW(
		0,
		PROGRESS_CLASSW,
		custom->title,
		WS_VISIBLE | WS_CHILD,
		(int)luaL_optnumber(L, 2, 0),
		(int)luaL_optnumber(L, 3, 0),
		(int)luaL_optnumber(L, 4, 0),
		(int)luaL_optnumber(L, 5, 0),
		window->handle,
		custom->hmenu,
		(HINSTANCE)GetWindowLongPtr(window->handle, GWLP_HINSTANCE),
		NULL);

	lua_pushvalue(L, 1);
	int refParent = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_pop(L, lua_gettop(L));

	LuaWindow* button = lua_pushwindow(L);
	button->handle = hwnd;
	button->custom = custom;
	button->custom->type = WINDOW_TYPE_PROGRESSBAR;
	button->custom->parentRef = refParent;

	AddLuaTableChild(L, window->custom);

	SendMessageW(hwnd, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessageW(hwnd, PBM_SETSTEP, (WPARAM)1, 0);

	return 1;
}