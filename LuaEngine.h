#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include "xp_lua_incl.h"

class LuaEngine
{
public:
	LuaEngine();
	~LuaEngine();

	char * RunFunction(const char * function, const char * param1, int param2, const char * value);
	char * RunString(const char * script, const char * name);
	const char * GetLastError();

	lua_State *L;
	bool AutoGC;
	bool Log;
private:
	char * _lasterror;
	void SetError(const char * err, size_t len=0);
	char * Luapcall(int params);
};

