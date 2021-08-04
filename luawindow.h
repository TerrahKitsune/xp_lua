#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
static const char* LUAWINDOW = "WINDOW";

typedef struct LuaWindow {

	HWND handle;
	bool wasCreated;

} LuaWindow;


LuaWindow* lua_pushwindow(lua_State* L);
LuaWindow* lua_tonwindow(lua_State* L, int index);

int OpenWindow(lua_State* L);
int GetWindowParent(lua_State* L);
int GetWindow(lua_State* L);
int GetIsVisible(lua_State* L);
int GetText(lua_State* L);
int GetWindowProcessId(lua_State* L);
int GetWindowInformation(lua_State* L);

int window_gc(lua_State* L);
int window_tostring(lua_State* L);