#pragma once
#include "lua_main_incl.h"
#include "luawindow.h"
#include <Windows.h>

void DoCustomButtonEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
int CreateCustomLuaButton(lua_State* L);