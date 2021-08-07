#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
static const char* LUAWINDOW = "WINDOW";

typedef struct LuaCustomWindow {

	int customDrawingRef;
	int threadRef;
	char* className;
	char* title;

} LuaCustomWindow;

typedef struct LuaWindow {

	HWND handle;
	LuaCustomWindow * custom;

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

int GetCustomWindowCoroutine(lua_State* L);
int CreateLuaWindow(lua_State* L);
int ShowCustomWindow(lua_State* L);
int LuaSetDrawFunction(lua_State* L);

int window_gc(lua_State* L);
int window_tostring(lua_State* L);