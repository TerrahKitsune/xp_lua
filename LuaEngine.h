#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>

#include "xp_lua_incl.h"

class LuaEngine
{
public:
	LuaEngine();
	~LuaEngine();

	bool RunString(char * result, size_t resultsize, const char * script, const char * name);

	lua_State *L;
};

