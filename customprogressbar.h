#pragma once
#include "lua_main_incl.h"
#include "luawindow.h"
#include <Windows.h>

int CreateCustomLuaProgressbar(lua_State* L);
int LuaProgressbarGetStep(lua_State* L);
int LuaProgressbarStep(lua_State* L);