#pragma once
#include "lua_main_incl.h"
extern "C" {
#include "sha256.h"
}
static const char * LUASHA256 = "LUASHA256";

typedef struct LuaSHA256 {
	SHA256_CTX SHA;
	unsigned char * hash;
} LuaSHA256;

LuaSHA256 * lua_tosha256(lua_State *L, int index);
LuaSHA256 * lua_pushsha256(lua_State *L);

int NewSHA256(lua_State *L);
int UpdateSHA256(lua_State *L);
int FinalSHA256(lua_State *L);

int sha256_gc(lua_State *L);
int sha256_tostring(lua_State *L);