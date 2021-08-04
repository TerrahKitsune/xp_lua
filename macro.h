#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
static const char* MACRO = "MACRO";

typedef struct LuaMacro {

	size_t length;
	INPUT* inputs;

} LuaMacro;


LuaMacro* lua_pushmacro(lua_State* L);
LuaMacro* lua_tomacro(lua_State* L, int index);

int CreateMacro(lua_State* L);
int SendMacro(lua_State* L);
int GetInputs(lua_State* L);
int ScreenToMouse(lua_State* L);

int macro_gc(lua_State* L);
int macro_tostring(lua_State* L);