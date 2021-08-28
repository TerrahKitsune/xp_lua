#pragma once
#include "lua_main_incl.h"
#include "luawindow.h"
#include <Windows.h>

void DoCustomComboBoxEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
void DoCustomListBoxEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
void DoCustomListViewEvent(lua_State* L, LuaWindow* parent, LuaWindow* child, HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
int CreateCustomLuaComboBox(lua_State* L);
int AddBoxItem(lua_State* L);
int DeleteBoxItem(lua_State* L);
int GetBoxItems(lua_State* L);
int ListviewSetItemText(lua_State* L);
int SetViewlistColumnWidth(lua_State* L);
int GetSelectedIndex(lua_State* L);
int SetSelectedIndex(lua_State* L);

int CreateCustomLuaListbox(lua_State* L);
int CreateCustomLuaListView(lua_State* L);