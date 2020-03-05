#include "LuaSha256.h"
#include <stdlib.h>
#include <string.h>

LuaSHA256 * lua_tosha256(lua_State *L, int index) {

	LuaSHA256 * luasha256 = (LuaSHA256*)luaL_checkudata(L, index, LUASHA256);
	if (luasha256 == NULL)
		luaL_error(L, "parameter is not a %s", LUASHA256);
	return luasha256;
}

LuaSHA256 * lua_pushsha256(lua_State *L) {

	LuaSHA256 * luasha256 = (LuaSHA256*)lua_newuserdata(L, sizeof(LuaSHA256));
	if (luasha256 == NULL)
		luaL_error(L, "Unable to create md5 instance");
	luaL_getmetatable(L, LUASHA256);
	lua_setmetatable(L, -2);
	memset(luasha256, 0, sizeof(LuaSHA256));
	return luasha256;
}

int NewSHA256(lua_State *L) {

	LuaSHA256 * luasha256 = lua_pushsha256(L);
	if (!luasha256)
		luaL_error(L, "Unable to push sha256 instance");

	sha256_init(&luasha256->SHA);

	return 1;
}

int UpdateSHA256(lua_State *L) {

	LuaSHA256 * luasha256 = lua_tosha256(L, 1);
	if (!luasha256)
		luaL_error(L, "Unable to get sha256 instance");
	else if (luasha256->hash)
		luaL_error(L, "Cannot update already finished sha256 digest");

	size_t len;
	const char * data = luaL_tolstring(L, 2, &len);
	if (data) {
		sha256_update(&luasha256->SHA, (unsigned char*)data, len);
	}
	lua_pop(L, 2);
	return 0;
}

int FinalSHA256(lua_State *L) {

	LuaSHA256 * luasha256 = lua_tosha256(L, 1);
	if (!luasha256->hash) {
		luasha256->hash = (unsigned char*)malloc(SHA256_BLOCK_SIZE);
		sha256_final(&luasha256->SHA, luasha256->hash);
	}

	char sha256string[(SHA256_BLOCK_SIZE*2) + 1];
	for (int i = 0; i < SHA256_BLOCK_SIZE; ++i)
		sprintf(&sha256string[i * 2], "%02x", (unsigned int)luasha256->hash[i]);

	lua_pop(L, 1);
	lua_pushstring(L, sha256string);
	lua_pushlstring(L, (const char*)luasha256->hash, SHA256_BLOCK_SIZE);

	return 2;
}

int sha256_gc(lua_State *L) {

	LuaSHA256 * luasha256 = lua_tosha256(L, 1);

	if (!luasha256->hash) {
		luasha256->hash = (unsigned char*)malloc(SHA256_BLOCK_SIZE);
		sha256_final(&luasha256->SHA, luasha256->hash);
	}

	free(luasha256->hash);
	luasha256->hash = NULL;

	return 0;
}

int sha256_tostring(lua_State *L) {

	char sha256[100];
	sprintf(sha256, "SHA256: 0x%08X", lua_tosha256(L, 1));
	lua_pushfstring(L, sha256);
	return 1;
}