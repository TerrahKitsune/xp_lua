#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
#include "aes.hpp"
static const char * LUAES = "LUAES";

#define AES_256_ECB 1
#define AES_256_CBC 2
#define AES_256_CTR 3

typedef struct LuaAes {

	DWORD type;
	uint8_t iv[AES_BLOCKLEN];
	AES_ctx context;

} LuaAes;


LuaAes * lua_pushluaaes(lua_State *L);
LuaAes * lua_toluaaes(lua_State *L, int index);

int LuaCreateContext(lua_State *L);
int LuaAesEncrypt(lua_State *L);
int LuaAesDecrypt(lua_State *L);
int LuaSetIV(lua_State *L);

int luaaes_gc(lua_State *L);
int luaaes_tostring(lua_State *L);