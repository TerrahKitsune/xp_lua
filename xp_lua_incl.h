//build it as a dll
#define LUA_BUILD_AS_DLL
//then embed the dll directly
#define LUA_CORE

#include "./lua/lua.hpp"