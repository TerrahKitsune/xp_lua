#pragma once
#include "lua_main_incl.h"
#include "NWN2Funcs.h"
#include "./NWN2_API/NWN2_API.h"

int GetCreature(lua_State*L);
int GetObject(lua_State*L);
int LRunScript(lua_State*L);
int Test(lua_State*L);

int EffectSetEffectInt(lua_State*L);
nwn_objid_t GetObjID(lua_State*L, int idx);
int CopyEffectIdsToEffectInts(lua_State*L);
int EffectSetExposed(lua_State*L);
int GetEffectData(lua_State*L);
int GetLocalVariables(lua_State*L);
int SetEffectString(lua_State*L); 
int SetEffectObject(lua_State*L);
