#pragma once
#include "lua_main_incl.h"
static const char * HTTPLUA = "HTTPLUA";

int HTTP(lua_State *L);
int SetTimeoutFunction(lua_State *L);
int SetFile(lua_State *L);