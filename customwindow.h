#pragma once
#include "lua_main_incl.h"
#include "luawindow.h"
#include <Windows.h>

#define		WM_LUA_DESTROY		(WM_USER + 1)
#define		WM_LUA_TOGGLESHOW	(WM_USER + 2)
#define		WM_LUA_UPDATE		(WM_USER + 3)
#define		WM_LUA_TOGGLEENABLE	(WM_USER + 4)

LRESULT CALLBACK WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

LuaCustomWindow* CreateCustomWindowStruct();
void AddLuaTableChild(lua_State* L, LuaCustomWindow* window);
size_t GetLuaChildrenCount(lua_State* L, LuaCustomWindow* window);
void CleanUp(LuaCustomWindow* custom);
int CreateLuaCustomWindow(lua_State* L);
int RemoveCustomWindow(lua_State* L);
int LuaShowCustomWindow(lua_State* L);
int LuaEnableCustomWindow(lua_State* L);

int LuaSetCustomWindowDrawFunction(lua_State* L);
int CreateCustomLuaButton(lua_State* L);
bool CheckHasMessage(LuaWindow* window);

int lua_customcoroutineiterator(lua_State* L, int status, lua_KContext ctx);
int lua_customwindowloop(lua_State* L);