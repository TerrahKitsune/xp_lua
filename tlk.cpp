#include "tlk.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

int tlk_open(lua_State *L){

	const char * file = luaL_checkstring(L, 1);
	FILE * f = fopen(file, "rb");
	if (!f){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}
	else
		lua_pop(L, lua_gettop(L));

	TlkHeader header;
	if (fread(&header, 1, sizeof(TlkHeader), f) != sizeof(TlkHeader)){
		fclose(f);
		lua_pushnil(L);
		return 1;
	}

	if (tolower(header.FileType[0]) != 't' ||
		tolower(header.FileType[1]) != 'l' ||
		tolower(header.FileType[2]) != 'k' ||
		tolower(header.FileType[3]) != ' ')
	{
		fclose(f);
		lua_pushnil(L);
		return 1;
	}

	LuaTLK * tlk = lua_pushtlk(L);

	tlk->file = f;
	memcpy(&tlk->Header, &header, sizeof(TlkHeader));

	return 1;
}

int tlk_info(lua_State *L){
	LuaTLK * tlk = (LuaTLK*)lua_totlk(L, 1);
	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, tlk->Header.StringCount);
	lua_pushinteger(L, tlk->Header.LanguageID);
	lua_pushlstring(L, tlk->Header.FileVersion, 4);
	return 3;
}

int tlk_get(lua_State *L)
{
	LuaTLK * tlk = (LuaTLK*)lua_totlk(L, 1);
	lua_Integer index = luaL_checkinteger(L, 2);

	if (index > tlk->Header.StringCount || index < 0){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	TlkStringData entry;
	rewind(tlk->file);
	if (fseek(tlk->file, sizeof(TlkHeader) + (sizeof(TlkStringData) * index), SEEK_SET) != 0){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	if (fread(&entry, 1, sizeof(TlkStringData), tlk->file) != sizeof(TlkStringData)){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	char * str;

	if (entry.Flags & TEXT_PRESENT){
		if (fseek(tlk->file, tlk->Header.StringEntriesOffset + entry.OffsetToString, SEEK_SET) != 0){
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			return 1;
		}

		str = (char*)malloc(entry.StringSize);
		if (!str){
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			return 1;
		}

		if (fread(str, 1, entry.StringSize, tlk->file) != entry.StringSize)
		{
			free(str);
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			return 1;
		}

		lua_pushlstring(L, str, entry.StringSize);
		free(str);
	}
	else{
		lua_pushstring(L, "");
	}

	return 1;
}

int tlk_getall(lua_State *L){

	LuaTLK * tlk = (LuaTLK*)lua_totlk(L, 1);
	
	rewind(tlk->file);
	if (fseek(tlk->file, sizeof(TlkHeader), SEEK_SET) != 0){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	long int prev;
	TlkStringData entry;
	char * str;

	lua_pop(L, lua_gettop(L));
	lua_createtable(L, tlk->Header.StringCount, 0);

	for (int n = 0; n < tlk->Header.StringCount; n++){

		if (fread(&entry, 1, sizeof(TlkStringData), tlk->file) != sizeof(TlkStringData)){
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			return 1;
		}

		if (entry.Flags & TEXT_PRESENT)
		{
			prev = ftell(tlk->file);
			if (fseek(tlk->file, tlk->Header.StringEntriesOffset + entry.OffsetToString, SEEK_SET) != 0){
				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				return 1;
			}

			str = (char*)malloc(entry.StringSize);
			if (!str){
				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				return 1;
			}

			if (fread(str, 1, entry.StringSize, tlk->file) != entry.StringSize)
			{
				free(str);
				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				return 1;
			}

			lua_pushlstring(L, str, entry.StringSize);
			free(str);
			fseek(tlk->file, prev, SEEK_SET);
		}
		else{
			lua_pushstring(L, "");
		}

		lua_rawseti(L, -2, n);
	}

	return 1;
}

LuaTLK * lua_pushtlk(lua_State *L){
	LuaTLK * tlk = (LuaTLK*)lua_newuserdata(L, sizeof(LuaTLK));
	if (tlk == NULL)
		luaL_error(L, "Unable to push tlk");
	luaL_getmetatable(L, TLK);
	lua_setmetatable(L, -2);
	memset(tlk, 0, sizeof(LuaTLK));
	return tlk;
}

LuaTLK * lua_totlk(lua_State *L, int index){
	LuaTLK * tlk = (LuaTLK*)luaL_checkudata(L, index, TLK);
	if (tlk == NULL)
		luaL_error(L, "paramter is not a %s", TLK);
	return tlk;
}

int tlk_gc(lua_State *L){

	LuaTLK * tlk = lua_totlk(L, 1);
	if (tlk->file){
		fclose(tlk->file);
		tlk = NULL;
	}
	return 0;
}

int tlk_tostring(lua_State *L){
	char tim[100];
	sprintf(tim, "Tlk: 0x%08X", lua_totlk(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}