#pragma once
#include "lua_main_incl.h"
#include "luawindow.h"
#include <Windows.h>

void DoCustomComboBoxEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
int CreateCustomLuaComboBox(lua_State* L);
int AddComboBoxItem(lua_State* L);
int DeleteComboBoxItem(lua_State* L);
int GetComboBoxItems(lua_State* L);