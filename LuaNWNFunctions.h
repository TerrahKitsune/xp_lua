#pragma once
#include "lua_main_incl.h"
#include "NWN2Funcs.h"
#include "./NWN2_API/NWN2_API.h"

int GetCreature(lua_State*L);
int LRunScript(lua_State*L);
int Test(lua_State*L);

nwn_objid_t GetObjID(lua_State*L, int idx);

