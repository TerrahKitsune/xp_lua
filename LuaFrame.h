#pragma once
#include "lua_main_incl.h"

typedef struct LuaFrameKey{
	int luatype;
	unsigned int keyoffset;
	unsigned int keylength;
	unsigned int dataoffset;
	unsigned int datalength;
}LuaFrameKey;

typedef struct LuaFrame {
	unsigned int length;
	char method[16];
	unsigned int numbkeys;
	LuaFrameKey keys[];
} LuaFrame;

LuaFrame * lua_toframe(lua_State *L, int idx, const char * method);
void lua_pushframe(lua_State *L, LuaFrame * frame);
void FreeLuaFrame(LuaFrame * frame);