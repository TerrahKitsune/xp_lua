#include "luaaes.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 
#include <windows.h> 

int LuaCreateContext(lua_State* L) {

	size_t keylen;
	size_t ivlen;
	const char* keystr = luaL_checklstring(L, 1, &keylen);
	const char* ivstr = luaL_optlstring(L, 2, NULL, &ivlen);

	uint8_t key[AES_KEYLEN];
	uint8_t iv[AES_BLOCKLEN];

	LuaAes* luaaes = lua_pushluaaes(L);

	if (keylen > AES_KEYLEN) {

		luaL_error(L, "Key length must be 32 bytes");
		return 0;
	}
	else {
		ZeroMemory(key, AES_KEYLEN);
		memcpy(key, keystr, keylen);
	}

	if (ivstr && ivlen > 0) {

		if (ivlen > AES_BLOCKLEN) {
			luaL_error(L, "IV length must be 16 bytes");
			return 0;
		}
		else {
			ZeroMemory(iv, AES_BLOCKLEN);
			memcpy(iv, ivstr, ivlen);
		}

		memcpy(luaaes->iv, iv, AES_BLOCKLEN);
		luaaes->type = (lua_isboolean(L, 3) && lua_toboolean(L, 3)) ? AES_256_CTR : AES_256_CBC;
		AES_init_ctx_iv(&luaaes->context, key, iv);
	}
	else {
		luaaes->type = AES_256_ECB;
		AES_init_ctx(&luaaes->context, key);
	}

	return 1;
}

int LuaSetIV(lua_State* L) {

	LuaAes* luaaes = lua_toluaaes(L, 1);
	size_t len;
	const char* data = lua_tolstring(L, 2, &len);

	if (luaaes->type == AES_256_ECB) {
		return 0;
	}
	else if (!data || len <= 0) {
		data = (const char*)luaaes->iv;
		len = AES_BLOCKLEN;
	}

	if (len > AES_BLOCKLEN) {
		luaL_error(L, "IV length must be 16 bytes");
		return 0;
	}

	AES_ctx_set_iv(&luaaes->context, (const uint8_t*)data);

	return 0;
}

int LuaAesEncrypt(lua_State* L) {

	LuaAes* luaaes = lua_toluaaes(L, 1);
	size_t len;
	const char* data = luaL_checklstring(L, 2, &len);

	size_t datalen = 16 * ((size_t)((double)len / AES_BLOCKLEN) + 1);

	uint8_t* cbcbuf = (uint8_t*)gff_calloc(datalen, sizeof(uint8_t));

	if (!cbcbuf) {
		luaL_error(L, "Unable to allocate buffer");
		return 0;
	}

	memcpy(cbcbuf, data, len);
	memset(&cbcbuf[len], (BYTE)(datalen - len), datalen - len);

	for (size_t i = 0; i < (datalen / AES_BLOCKLEN); i++)
	{
		switch (luaaes->type) {

		case AES_256_ECB:
			AES_ECB_encrypt(&luaaes->context, &cbcbuf[i * AES_BLOCKLEN]);
			break;

		case AES_256_CBC:
			AES_CBC_encrypt_buffer(&luaaes->context, &cbcbuf[i * AES_BLOCKLEN], AES_BLOCKLEN);
			break;

		case AES_256_CTR:
			AES_CTR_xcrypt_buffer(&luaaes->context, &cbcbuf[i * AES_BLOCKLEN], AES_BLOCKLEN);
			break;
		}
	}

	lua_pop(L, lua_gettop(L));
	lua_pushlstring(L, (const char*)cbcbuf, datalen);

	gff_free(cbcbuf);

	return 1;
}

int LuaAesDecrypt(lua_State* L) {

	LuaAes* luaaes = lua_toluaaes(L, 1);
	size_t len;
	const char* data = luaL_checklstring(L, 2, &len);

	size_t datalen = len;

	if (datalen % 16 != 0) {
		luaL_error(L, "Aes decrypt invalid blocksize");
		return 0;
	}

	uint8_t* cbcbuf = (uint8_t*)gff_calloc(datalen, sizeof(uint8_t));

	if (!cbcbuf) {
		luaL_error(L, "Unable to allocate buffer");
		return 0;
	}

	memcpy(cbcbuf, data, len);

	for (size_t i = 0; i < (datalen / AES_BLOCKLEN); i++)
	{
		switch (luaaes->type) {

			case AES_256_ECB:
				AES_ECB_decrypt(&luaaes->context, &cbcbuf[i * AES_BLOCKLEN]);
				break;

			case AES_256_CBC:
				AES_CBC_decrypt_buffer(&luaaes->context, &cbcbuf[i * AES_BLOCKLEN], AES_BLOCKLEN);
				break;

			case AES_256_CTR:
				AES_CTR_xcrypt_buffer(&luaaes->context, &cbcbuf[i * AES_BLOCKLEN], AES_BLOCKLEN);
				break;
		}
	}

	BYTE paddingbyte = cbcbuf[len - 1];

	if (paddingbyte <= 0 || paddingbyte > AES_BLOCKLEN) {
		gff_free(cbcbuf);
		luaL_error(L, "Invalid aes padding");
		return 0;
	}
	else {
		datalen -= paddingbyte;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushlstring(L, (const char*)cbcbuf, datalen);

	gff_free(cbcbuf);

	return 1;
}

LuaAes* lua_pushluaaes(lua_State* L) {

	LuaAes* luaaes = (LuaAes*)lua_newuserdata(L, sizeof(LuaAes));

	if (luaaes == NULL)
		luaL_error(L, "Unable to push aes");
	luaL_getmetatable(L, LUAES);
	lua_setmetatable(L, -2);

	memset(luaaes, 0, sizeof(LuaAes));

	return luaaes;
}

LuaAes* lua_toluaaes(lua_State* L, int index) {
	LuaAes* luaaes = (LuaAes*)luaL_checkudata(L, index, LUAES);
	if (luaaes == NULL)
		luaL_error(L, "parameter is not a %s", LUAES);
	return luaaes;
}

int luaaes_gc(lua_State* L) {

	LuaAes* luaaes = lua_toluaaes(L, 1);

	return 0;
}

int luaaes_tostring(lua_State* L) {
	
	char tim[100];
	LuaAes* luaaes = lua_toluaaes(L, 1);

	switch (luaaes->type) {

	case AES_256_ECB:
		sprintf(tim, "LuaAes: 0x%08X (aes-256-ecb)", luaaes);
		break;

	case AES_256_CBC:
		sprintf(tim, "LuaAes: 0x%08X (aes-256-cbc)", luaaes);
		break;

	case AES_256_CTR:
		sprintf(tim, "LuaAes: 0x%08X (aes-256-ctr)", luaaes);
		break;

	default:
		sprintf(tim, "LuaAes: 0x%08X (invalid)", luaaes);
		break;
	}

	lua_pushstring(L, tim);
	return 1;
}