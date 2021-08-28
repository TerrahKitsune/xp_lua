#pragma once
#include "lua_main_incl.h"
#include "luawindow.h"
#include <Windows.h>

void DoCustomTextboxEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
int CreateTextField(lua_State* L);
int CreateStaticTextField(lua_State* L);