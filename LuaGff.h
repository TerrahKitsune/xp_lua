#pragma once
#include "lua_main_incl.h"

int OpenGffFile(lua_State *L);
int OpenGffString(lua_State *L);
int SaveGffToFile(lua_State *L);
int SaveGffToString(lua_State *L);