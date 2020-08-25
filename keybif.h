#pragma once
#include "lua_main_incl.h"
static const char* KEYBIF = "KEYBIF";

typedef struct LuaKeyBif {

} LuaKeyBif;

int CreateKeyBif(lua_State* L);

typedef struct FileEntry {

	char* Key;
	void* Data;

} FileEntry;

int CreateKeyBif(lua_State* L);

LuaKeyBif* lua_pushkeybif(lua_State* L);
LuaKeyBif* lua_tokeybif(lua_State* L, int index);

int keybif_gc(lua_State* L);
int keybif_tostring(lua_State* L);