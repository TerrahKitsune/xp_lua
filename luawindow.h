#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
static const char* LUAWINDOW = "WINDOW";

#define WINDOW_TYPE_CUSTOM 0
#define WINDOW_TYPE_BUTTON 1
#define WINDOW_TYPE_TEXTBOX 2

typedef struct LuaCustomWindow {

	int customDrawingRef;
	int threadRef;
	int childRef;
	int parentRef;
	int eventRef;
	char* className;
	char* title;
	DWORD type;
	WORD nextId;
	HMENU hmenu;

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
int LuaWindowGetId(lua_State* L);
int LuaDestroyWindow(lua_State* L);
int LuaGetContent(lua_State* L);
int LuaSetContent(lua_State* L);
int GetsWindowEnabled(lua_State* L);

int LuaCreateCustomTextField(lua_State* L);
int InvalidateWindow(lua_State* L);
int GetCustomWindowCoroutine(lua_State* L);
int CreateLuaWindow(lua_State* L);
int ShowCustomWindow(lua_State* L);
int LuaSetDrawFunction(lua_State* L);
int LuaCheckHasMessage(lua_State* L);
int CreateLuaButton(lua_State* L);

int window_gc(lua_State* L);
int window_tostring(lua_State* L);