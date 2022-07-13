#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
static const char* LUAWCHAR = "WCHAR";

typedef struct LuaWChar {

	size_t len;
	wchar_t* str;

} LuaWChar;

LuaWChar* lua_pushwchar(lua_State* L, const wchar_t* str);
LuaWChar* lua_pushwchar(lua_State* L, const wchar_t* str, size_t len);
LuaWChar* lua_towchar(lua_State* L, int index);
LuaWChar* lua_pushwchar(lua_State* L);
LuaWChar* lua_stringtowchar(lua_State* L, int index);

int FromAnsi(lua_State* L);
int ToAnsi(lua_State* L);
int ToWide(lua_State* L);
int FromSubstring(lua_State* L);
int FromToLower(lua_State* L);
int FromToUpper(lua_State* L);
int WcharFind(lua_State* L);
int ToBytes(lua_State* L);
int FromBytes(lua_State* L);

int wchar_len(lua_State* L);
int wchar_eq(lua_State* L);
int wchar_concat(lua_State* L);
int wchar_gc(lua_State* L);
int wchar_tostring(lua_State* L);