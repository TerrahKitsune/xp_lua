#include "luawindow.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 
#include "customwindow.h"
#include "custombutton.h"

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam);

struct handle_data {
	lua_State* L;
	unsigned long process_id;
	int i;
	bool all;
};

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
	handle_data& data = *(handle_data*)lParam;
	unsigned long process_id = 0;

	if (!data.all) {
		GetWindowThreadProcessId(handle, &process_id);
	}

	if (data.all || data.process_id == process_id) {

		LuaWindow* window = lua_pushwindow(data.L);
		window->handle = handle;
		lua_rawseti(data.L, -2, ++data.i);
	}

	return TRUE;
}

int CreateLuaWindow(lua_State* L) {

	return CreateLuaCustomWindow(L);
}

int CreateLuaButton(lua_State* L) {

	return CreateCustomLuaButton(L);
}

int LuaSetDrawFunction(lua_State* L) {

	return LuaSetCustomWindowDrawFunction(L);
}

int InvalidateWindow(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (window->custom && window->custom->type == WINDOW_TYPE_CUSTOM) {
		lua_pushboolean(L, RedrawWindow(window->handle, NULL, NULL, RDW_INVALIDATE) > 0);
	}
	else {
		lua_pushboolean(L, false);
	}

	return 1;
}

int GetCustomWindowCoroutine(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	lua_pop(L, lua_gettop(L));

	if (!window->custom || window->custom->threadRef == LUA_REFNIL) {
		lua_pushnil(L);
		return 1;
	}
	else {
		lua_rawgeti(L, LUA_REGISTRYINDEX, window->custom->threadRef);
		return 1;
	}
}

int ShowCustomWindow(lua_State* L) {

	return LuaShowCustomWindow(L);
}

int OpenWindow(lua_State* L) {

	handle_data data;

	if (lua_type(L, 1) == LUA_TNUMBER) {
		data.process_id = (unsigned long)luaL_checkinteger(L, 1);
		data.all = false;
	}
	else {
		data.all = true;
	}

	data.L = L;
	data.i = 0;

	lua_pop(L, lua_gettop(L));

	lua_newtable(L);

	EnumWindows(enum_windows_callback, (LPARAM)&data);

	return 1;
}

int LuaCheckHasMessage(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	lua_pushboolean(L, CheckHasMessage(window));

	return 1;
}

int GetIsVisible(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, IsWindowVisible(window->handle));

	return 1;
}

int GetsWindowEnabled(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, IsWindowEnabled(window->handle));

	return 1;
}

void PushRect(lua_State* L, RECT rect) {

	lua_createtable(L, 0, 4);

	lua_pushstring(L, "Bottom");
	lua_pushinteger(L, rect.bottom);
	lua_settable(L, -3);

	lua_pushstring(L, "Left");
	lua_pushinteger(L, rect.left);
	lua_settable(L, -3);

	lua_pushstring(L, "Right");
	lua_pushinteger(L, rect.right);
	lua_settable(L, -3);

	lua_pushstring(L, "Top");
	lua_pushinteger(L, rect.top);
	lua_settable(L, -3);
}

void PushWindowInfo(lua_State* L, WINDOWINFO info) {

	lua_createtable(L, 0, 9);

	lua_pushstring(L, "AtomWindowType");
	lua_pushinteger(L, info.atomWindowType);
	lua_settable(L, -3);

	lua_pushstring(L, "XWindowBorders");
	lua_pushinteger(L, info.cxWindowBorders);
	lua_settable(L, -3);

	lua_pushstring(L, "YWindowBorders");
	lua_pushinteger(L, info.cyWindowBorders);
	lua_settable(L, -3);

	lua_pushstring(L, "ExStyle");
	lua_pushinteger(L, info.dwExStyle);
	lua_settable(L, -3);

	lua_pushstring(L, "Style");
	lua_pushinteger(L, info.dwStyle);
	lua_settable(L, -3);

	lua_pushstring(L, "WindowStatus");
	lua_pushinteger(L, info.dwWindowStatus);
	lua_settable(L, -3);

	lua_pushstring(L, "CreatorVersion");
	lua_pushinteger(L, info.wCreatorVersion);
	lua_settable(L, -3);

	lua_pushstring(L, "Client");
	PushRect(L, info.rcClient);
	lua_settable(L, -3);

	lua_pushstring(L, "Window");
	PushRect(L, info.rcWindow);
	lua_settable(L, -3);
}

int GetWindowInformation(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	WINDOWINFO info;
	ZeroMemory(&info, sizeof(WINDOWINFO));
	info.cbSize = sizeof(WINDOWINFO);

	lua_pop(L, lua_gettop(L));

	if (GetWindowInfo(window->handle, &info)) {
		PushWindowInfo(L, info);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int LuaDestroyWindow(lua_State* L) {
	return RemoveCustomWindow(L);
}

int GetWindow(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	HWND hwnd = GetWindow(window->handle, (UINT)luaL_checkinteger(L, 2));

	lua_pop(L, lua_gettop(L));

	if (hwnd != NULL) {
		window = lua_pushwindow(L);
		window->handle = hwnd;
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int GetWindowParent(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (window->custom && window->custom->parentRef != LUA_REFNIL) {

		lua_rawgeti(L, LUA_REGISTRYINDEX, window->custom->parentRef);
		return 1;
	}

	HWND hwnd = GetParent(window->handle);

	lua_pop(L, lua_gettop(L));

	if (hwnd != NULL) {
		window = lua_pushwindow(L);
		window->handle = hwnd;
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int GetWindowProcessId(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	lua_pop(L, lua_gettop(L));

	DWORD processId;
	DWORD threadId = GetWindowThreadProcessId(window->handle, &processId);

	lua_pushinteger(L, processId);
	lua_pushinteger(L, threadId);

	return 2;
}

int GetText(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	int len = GetWindowTextLength(window->handle);

	lua_pop(L, lua_gettop(L));

	if (len <= 0) {
		lua_pushstring(L, "");
		return 1;
	}

	char* str = (char*)gff_calloc(len + 1, sizeof(char));

	if (!str) {
		lua_pushstring(L, "");
		return 1;
	}

	GetWindowText(window->handle, str, len + 1);

	lua_pushlstring(L, str, len);

	gff_free(str);

	return 1;
}

int LuaWindowGetId(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);
	lua_pushinteger(L, (lua_Integer)window->handle);

	return 1;
}

int LuaSetContent(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	SetWindowText(window->handle, lua_tostring(L, 2));

	return 0;
}

int LuaGetContent(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	size_t len = GetWindowTextLength(window->handle);

	if (len == 0) {
		lua_pushstring(L, "");
		return 1;
	}

	char* data = (char*)gff_calloc(len+1, sizeof(char));

	if (!data) {
		luaL_error(L, "Out of memory");
		return 0;
	}

	int ret = GetWindowText(window->handle, data, len+1);

	lua_pushlstring(L, data, ret);

	gff_free(data);

	return 1;
}

LuaWindow* lua_pushwindow(lua_State* L) {
	LuaWindow* window = (LuaWindow*)lua_newuserdata(L, sizeof(LuaWindow));
	if (window == NULL)
		luaL_error(L, "Unable to push window");
	luaL_getmetatable(L, LUAWINDOW);
	lua_setmetatable(L, -2);
	memset(window, 0, sizeof(LuaWindow));

	return window;
}

LuaWindow* lua_tonwindow(lua_State* L, int index) {
	LuaWindow* window = (LuaWindow*)luaL_checkudata(L, index, LUAWINDOW);
	if (window == NULL)
		luaL_error(L, "parameter is not a %s", LUAWINDOW);
	return window;
}

int window_gc(lua_State* L) {

	LuaWindow* window = lua_tonwindow(L, 1);

	if (window->custom) {

		if (window->custom->threadRef != LUA_REFNIL) {
			luaL_unref(L, LUA_REGISTRYINDEX, window->custom->threadRef);
		}

		if (window->custom->customDrawingRef != LUA_REFNIL) {
			luaL_unref(L, LUA_REGISTRYINDEX, window->custom->customDrawingRef);
		}

		if (window->custom->childRef != LUA_REFNIL) {
			luaL_unref(L, LUA_REGISTRYINDEX, window->custom->childRef);
		}

		if (window->custom->eventRef != LUA_REFNIL) {
			luaL_unref(L, LUA_REGISTRYINDEX, window->custom->eventRef);
		}

		if (window->custom->parentRef != LUA_REFNIL) {
			luaL_unref(L, LUA_REGISTRYINDEX, window->custom->parentRef);
		}

		DestroyWindow(window->handle);

		CleanUp(window->custom);
	}

	ZeroMemory(window, sizeof(LuaWindow));

	return 0;
}

int window_tostring(lua_State* L) {
	char tim[100];
	sprintf(tim, "Window: 0x%08X", lua_tonwindow(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}