#include "base64.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char encoding_table[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
'w', 'x', 'y', 'z', '0', '1', '2', '3',
'4', '5', '6', '7', '8', '9', '+', '/' };
static char decoding_table[256];
static int mod_table[] = { 0, 2, 1 };


char *base64_encode(const unsigned char *data,
	size_t input_length,
	size_t *output_length) {

	*output_length = 4 * ((input_length + 2) / 3);

	char *encoded_data = (char *)gff_malloc(*output_length);
	if (encoded_data == NULL) return NULL;

	for (int i = 0, j = 0; i < input_length;) {

		uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
		uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
		uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
	}

	for (int i = 0; i < mod_table[input_length % 3]; i++)
		encoded_data[*output_length - 1 - i] = '=';

	return encoded_data;
}


unsigned char *base64_decode(const char *data,
	size_t input_length,
	size_t *output_length) {

	if (input_length % 4 != 0) return NULL;

	*output_length = input_length / 4 * 3;
	if (data[input_length - 1] == '=') (*output_length)--;
	if (data[input_length - 2] == '=') (*output_length)--;

	unsigned char *decoded_data = (unsigned char *)gff_malloc(*output_length);
	if (decoded_data == NULL) return NULL;

	for (int i = 0, j = 0; i < input_length;) {

		uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

		uint32_t triple = (sextet_a << 3 * 6)
			+ (sextet_b << 2 * 6)
			+ (sextet_c << 1 * 6)
			+ (sextet_d << 0 * 6);

		if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
		if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
		if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
	}

	return decoded_data;
}


void build_decoding_table() {

	for (int i = 0; i < 64; i++) {
		decoding_table[(unsigned char)encoding_table[i]] = i;
	}
}

int lua_base64encode(lua_State *L) {

	size_t len;
	const char * data = luaL_checklstring(L, 1, &len);
	
	size_t encodedlen;
	char* encoded = base64_encode((const unsigned char*)data, len, &encodedlen);

	if (encoded) {
		lua_pushlstring(L, encoded, encodedlen);
		gff_free(encoded);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int lua_base64decode(lua_State *L) {

	size_t len;
	const char * data = luaL_checklstring(L, 1, &len);

	size_t encodedlen;
	unsigned char* decoded = base64_decode(data, len, &encodedlen);

	if (decoded) {
		lua_pushlstring(L, (const char*)decoded, encodedlen);
		gff_free(decoded);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int lua_base64getencodingtable(lua_State *L) {

	lua_pushlstring(L, encoding_table, 64);
	return 1;
}

int lua_base64setencodingtable(lua_State *L) {

	size_t len;
	const char * data = luaL_checklstring(L, 1, &len);

	if (len != 64) {
		luaL_error(L, "Encoding table must be 64 bytes long");
		return 0;
	}

	for (size_t x = 0; x < len; x++)
	{
		for (size_t y = x+1; y < len; y++)
		{
			if (data[x] == data[y]) {
				luaL_error(L, "Entries in the encoding table must be unique");
				return 0;
			}
		}
	}

	memcpy(&encoding_table[0], data, 64);
	build_decoding_table();

	return 0;
}

int luaopen_base64(lua_State *L) {

	build_decoding_table();
	lua_newtable(L);

	lua_pushstring(L, "Encode");
	lua_pushcfunction(L, lua_base64encode);
	lua_settable(L, -3);

	lua_pushstring(L, "Decode");
	lua_pushcfunction(L, lua_base64decode);
	lua_settable(L, -3);

	lua_pushstring(L, "GetEncodeTable");
	lua_pushcfunction(L, lua_base64getencodingtable);
	lua_settable(L, -3);

	lua_pushstring(L, "SetEncodeTable");
	lua_pushcfunction(L, lua_base64setencodingtable);
	lua_settable(L, -3);

	return 1;
}