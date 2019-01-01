#pragma once
#include "NWN2_API/NWN2_API.h"
#include "lua_main_incl.h"

void lua_pushlocation(lua_State*L, Location loc);
void lua_pushvector(lua_State*L, Vector vec);
void lua_pushobject(lua_State*L, nwn_objid_t obj);
void lua_pushcexostring(lua_State*L, CExoString* str);