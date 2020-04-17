#include "LuaMD5.h"
#include <stdlib.h>
#include <string.h>

LuaMD5 * lua_tomd5(lua_State *L, int index){

	LuaMD5 * luamd5 = (LuaMD5*)luaL_checkudata(L, index, LUAMD5);
	if (luamd5 == NULL)
		luaL_error(L, "parameter is not a %s", LUAMD5);
	return luamd5;
}

LuaMD5 * lua_pushmd5(lua_State *L){

	LuaMD5 * luamd5 = (LuaMD5*)lua_newuserdata(L, sizeof(LuaMD5));
	if (luamd5 == NULL)
		luaL_error(L, "Unable to create md5 instance");
	luaL_getmetatable(L, LUAMD5);
	lua_setmetatable(L, -2);
	memset(luamd5, 0, sizeof(LuaMD5));
	return luamd5;
}

int NewMD5(lua_State *L){

	LuaMD5 * luamd5 = lua_pushmd5(L);
	if (!luamd5)
		luaL_error(L, "Unable to push md5 instance");

	MD5Init(&luamd5->MD5);

	return 1;
}

int UpdateMD5(lua_State *L){

	LuaMD5 * luamd5 = lua_tomd5(L, 1);
	if (!luamd5)
		luaL_error(L, "Unable to get md5 instance");
	else if (luamd5->hash)
		luaL_error(L, "Cannot update already finished md5 digest");

	size_t len;
	const char * data = luaL_tolstring(L, 2, &len);
	if (data){
		MD5Update(&luamd5->MD5, (unsigned char*)data, len);
	}
	lua_pop(L, 2);
	return 0;
}

int FinalMD5(lua_State *L){

	LuaMD5 * luamd5 = lua_tomd5(L, 1);
	if (!luamd5->hash){
		luamd5->hash = (unsigned char*)gff_malloc(16);
		MD5Final(luamd5->hash, &luamd5->MD5);
	}

	char md5string[33];
	for (int i = 0; i < 16; ++i)
		sprintf(&md5string[i * 2], "%02x", (unsigned int)luamd5->hash[i]);

	lua_pop(L, 1);
	lua_pushstring(L,md5string);
	lua_pushlstring(L, (const char*)luamd5->hash, 16);

	return 2;
}

int md5_gc(lua_State *L){

	LuaMD5 * luamd5 = lua_tomd5(L, 1);
	if (!luamd5->hash){
		luamd5->hash = (unsigned char*)gff_malloc(16);
		MD5Final(luamd5->hash, &luamd5->MD5);
	}

	gff_free(luamd5->hash);
	luamd5->hash = NULL;

	return 0;
}

int md5_tostring(lua_State *L){

	char md5s[100];
	sprintf(md5s, "MD5: 0x%08X", lua_tomd5(L, 1));
	lua_pushfstring(L, md5s);
	return 1;
}