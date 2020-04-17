#pragma once
#include "lua_main_incl.h"
extern "C" { 
#include "md5.h"
}
static const char * LUAMD5 = "MD5";

typedef struct LuaMD5 {
	MD5_CTX MD5;
	unsigned char hash[16];
} LuaMD5;

LuaMD5 * lua_tomd5(lua_State *L, int index);
LuaMD5 * lua_pushmd5(lua_State *L);

int NewMD5(lua_State *L);
int UpdateMD5(lua_State *L);
int FinalMD5(lua_State *L);

int md5_gc(lua_State *L);
int md5_tostring(lua_State *L);