#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include "xp_lua_incl.h"

class LuaEngine
{
public:
	LuaEngine();
	~LuaEngine();

	char * RunString(const char * script, const char * name);

	lua_State *L;
};

