#include "luaaes.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 

int LuaCreateContext(lua_State *L) {

	size_t keylen;
	size_t ivlen;
	const uint8_t * key = (const uint8_t *)luaL_checklstring(L, 1, &keylen);
	const uint8_t * iv = (const uint8_t *)luaL_optlstring(L, 2, NULL, &ivlen);

	LuaAes * luaaes = lua_pushluaaes(L);

	if (keylen != AES_KEYLEN) {

		luaL_error(L, "Key length must be 32 bytes");
		return 0;
	}

	if (iv) {
		
		if (ivlen != AES_BLOCKLEN) {
			luaL_error(L, "IV length must be 16 bytes");
			return 0;
		}

		memcpy(luaaes->iv, iv, AES_BLOCKLEN);
		luaaes->isEcb = false;
		AES_init_ctx_iv(&luaaes->context, key, iv);
	}
	else {
		luaaes->isEcb = true;
		AES_init_ctx(&luaaes->context, key);
	}

	return 1;
}

int LuaSetIV(lua_State *L) {

	LuaAes * luaaes = lua_toluaaes(L, 1);
	size_t len;
	const char * data = luaL_checklstring(L, 2, &len);

	if (len != AES_BLOCKLEN) {
		luaL_error(L, "IV length must be 16 bytes");
		return 0;
	}

	luaaes->isEcb = false;
	AES_ctx_set_iv(&luaaes->context, (const uint8_t*)data);

	return 0;
}

int LuaAesEncrypt(lua_State *L) {

	LuaAes * luaaes = lua_toluaaes(L, 1);
	size_t len;
	const char * data = luaL_checklstring(L, 2, &len);

	if (luaaes->isEcb) {

		if (len > AES_BLOCKLEN) {
			luaL_error(L, "Data is larger than 16 bytes");
			return 0;
		}

		uint8_t ecbbuf[AES_BLOCKLEN];

		ZeroMemory(ecbbuf, AES_BLOCKLEN);
		memcpy(ecbbuf, data, len);
		AES_ECB_encrypt(&luaaes->context, ecbbuf);
		
		lua_pop(L, lua_gettop(L));
		lua_pushlstring(L, (const char*)ecbbuf, AES_BLOCKLEN);
	}
	else {

		uint8_t* cbcbuf = (uint8_t*)calloc(len, sizeof(uint8_t));

		if (!cbcbuf) {
			luaL_error(L, "Unable to allocate buffer");
			return 0;
		}

		memcpy(cbcbuf, data, len);

		AES_CBC_encrypt_buffer(&luaaes->context, cbcbuf, len);

		lua_pop(L, lua_gettop(L));
		lua_pushlstring(L, (const char*)cbcbuf, len);

		free(cbcbuf);
	}

	return 1;
}

int LuaAesDecrypt(lua_State *L) {

	LuaAes * luaaes = lua_toluaaes(L, 1);
	size_t len;
	const char * data = luaL_checklstring(L, 2, &len);

	if (luaaes->isEcb) {

		if (len > AES_BLOCKLEN) {
			luaL_error(L, "Data is larger than 16 bytes");
			return 0;
		}

		uint8_t ecbbuf[AES_BLOCKLEN];

		ZeroMemory(ecbbuf, AES_BLOCKLEN);
		memcpy(ecbbuf, data, len);
		AES_ECB_decrypt(&luaaes->context, ecbbuf);

		lua_pop(L, lua_gettop(L));
		lua_pushlstring(L, (const char*)ecbbuf, AES_BLOCKLEN);
	}
	else {

		uint8_t* cbcbuf = (uint8_t*)calloc(len, sizeof(uint8_t));

		if (!cbcbuf) {
			luaL_error(L, "Unable to allocate buffer");
			return 0;
		}

		memcpy(cbcbuf, data, len);

		AES_CBC_decrypt_buffer(&luaaes->context, cbcbuf, len);

		lua_pop(L, lua_gettop(L));
		lua_pushlstring(L, (const char*)cbcbuf, len);

		free(cbcbuf);
	}

	return 1;
}

LuaAes * lua_pushluaaes(lua_State *L) {

	LuaAes * luaaes = (LuaAes*)lua_newuserdata(L, sizeof(LuaAes));

	if (luaaes == NULL)
		luaL_error(L, "Unable to push aes");
	luaL_getmetatable(L, LUAES);
	lua_setmetatable(L, -2);

	memset(luaaes, 0, sizeof(LuaAes));

	return luaaes;
}

LuaAes * lua_toluaaes(lua_State *L, int index) {
	LuaAes * luaaes = (LuaAes*)luaL_checkudata(L, index, LUAES);
	if (luaaes == NULL)
		luaL_error(L, "parameter is not a %s", LUAES);
	return luaaes;
}

int luaaes_gc(lua_State *L) {

	LuaAes * luaaes = lua_toluaaes(L, 1);

	return 0;
}

int luaaes_tostring(lua_State *L) {
	char tim[100];
	sprintf(tim, "LuaAes: 0x%08X", lua_toluaaes(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}