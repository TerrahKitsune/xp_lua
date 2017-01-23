#include "2da.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static int advance(const char * str){
	int n = 0;
	while (true){
		if (str[n] != ' ' && str[n] != '\t')
			return n;
		n++;
	}
	return n;
}

static int endofstring(const char * str){
	int n = 0;
	bool untilquote = str[0] == '"';

	if (untilquote){
		n++;
	}

	while (true){
		if (untilquote){
			if (str[n] == '"')
				return n;
		}
		else if (str[n] == ' ' || str[n] == '\t' || str[n] == '\r' || str[n] == '\n')
			return n;
		n++;
	}
	return n;
}

int twoda_open(lua_State *L){

	const char * file = luaL_checkstring(L, 1);
	FILE * f = fopen(file, "rb");
	if (!f){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}
	else
		lua_pop(L, lua_gettop(L));

	Lua2da header;
	memset(&header, NULL, sizeof(Lua2da));
	if (fread(&header, 1, 8, f) != 8){
		fclose(f);
		lua_pushnil(L);
		return 1;
	}

	if (tolower(header.FileType[0]) != '2' ||
		tolower(header.FileType[1]) != 'd' ||
		tolower(header.FileType[2]) != 'a' )
	{
		fclose(f);
		lua_pushnil(L);
		return 1;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);

	char * raw = (char*)malloc(size + 1);
	if (!raw){
		fclose(f);
		lua_pushnil(L);
		return 1;
	}
	raw[size] = '\0';
	if (fread(raw, 1, size, f) != size){
		free(raw);
		fclose(f);
		lua_pushnil(L);
		return 1;
	}

	fclose(f);

	char * cursor = strstr(raw, "\n");
	for (int n = 0; n < 2; n++){
		if (!cursor){
			free(raw);
			lua_pushnil(L);
			return 1;
		}
		cursor = strstr(cursor, "\n") + 1;
	}

	Lua2da * twoda = lua_pushtwoda(L);
	memcpy(twoda, &header, 8);
	//prase columns
	char * subcursor = cursor;
	char marker;
	int start;
	int end;
	do{
		start = advance(subcursor);
		marker = subcursor[start];
		if (marker == '\0' || marker == '\r' || marker == '\n')
			break;
		twoda->numbcols++;
		subcursor = &subcursor[start];
		end = endofstring(subcursor);
		subcursor = &subcursor[end + 1];
	} while (true);

	twoda->columns = (char**)calloc(twoda->numbcols, sizeof(char*));
	if (!twoda->columns){
		free(raw);
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}

	int n = 0;
	subcursor = cursor;
	do{
		start = advance(subcursor);
		marker = subcursor[start];
		if (marker == '\0' || marker == '\r' || marker == '\n')
			break;
		subcursor = &subcursor[start];
		end = endofstring(subcursor);

		twoda->columns[n] = (char*)malloc(end + 1);
		if (!twoda->columns[n]){
			free(raw);
			lua_pop(L, 1);
			lua_pushnil(L);
			return 1;
		}
		else
			memset(twoda->columns[n], 0, end + 1);

		if (marker == '"')
			memcpy(twoda->columns[n], subcursor + 1, end - 1);
		else
			memcpy(twoda->columns[n], subcursor, end);

		n++;
		subcursor = &subcursor[end + 1];
	} while (true);

	cursor = strstr(cursor, "\n");
	for (int n = 0; n < 1; n++){
		if (!cursor){
			free(raw);
			lua_pushnil(L);
			return 1;
		}
		cursor = strstr(cursor, "\n") + 1;
	}

	char * prev;
	subcursor = cursor;
	while (subcursor){
		prev = subcursor;
		subcursor = strstr(subcursor + 1, "\n");
		if (subcursor && subcursor - prev > 1)
			twoda->numbrows++;
	}

	twoda->rows = (char***)calloc(twoda->numbrows, sizeof(char**));
	if (!twoda->rows){
		free(raw);
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}

	char ** currentrow;
	int col = 0;
	bool first;
	while (cursor)
	{
		n = 0;
		subcursor = cursor;
		first = true;

		do{
			start = advance(subcursor);
			marker = subcursor[start];
			if (marker == '\0' || marker == '\r' || marker == '\n')
				break;
			subcursor = &subcursor[start];
			end = endofstring(subcursor);

			if (first){
				first = false;
				twoda->rows[col] = (char**)calloc(twoda->numbcols, sizeof(char*));
				if (!twoda->rows[col]){
					free(raw);
					lua_pop(L, 1);
					lua_pushnil(L);
					return 1;
				}
				else{
					currentrow = twoda->rows[col];
				}
			}
			else{
				currentrow[n] = (char*)malloc(end + 1);
				if (!currentrow[n]){
					free(raw);
					lua_pop(L, 1);
					lua_pushnil(L);
					return 1;
				}
				else
					memset(currentrow[n], 0, end + 1);

				if (marker == '"')
					memcpy(currentrow[n], subcursor + 1, end - 1);
				else
					memcpy(currentrow[n], subcursor, end);

				n++;
			}
			subcursor = &subcursor[end + 1];
		} while (true);

		col++;
		prev = cursor;
		cursor = strstr(cursor + 1, "\n");
		if (cursor)
			cursor++;
	}
	free(raw);

	return 1;
}

Lua2da * lua_pushtwoda(lua_State *L){
	Lua2da * twoda = (Lua2da*)lua_newuserdata(L, sizeof(Lua2da));
	if (twoda == NULL)
		luaL_error(L, "Unable to push 2da");
	luaL_getmetatable(L, TWODA);
	lua_setmetatable(L, -2);
	memset(twoda, 0, sizeof(Lua2da));
	return twoda;
}

Lua2da * lua_totwoda(lua_State *L, int index){
	Lua2da * tlk = (Lua2da*)luaL_checkudata(L, index, TWODA);
	if (tlk == NULL)
		luaL_error(L, "parameter is not a %s", TWODA);
	return tlk;
}

int twoda_get2dastring(lua_State *L){

	Lua2da * twoda = lua_totwoda(L, 1);
	int row = luaL_checkinteger(L, 2);
	int col = -1;

	if (lua_type(L, 3) == LUA_TNUMBER){
		col = lua_tointeger(L, 3);
	}
	else if (lua_type(L, 3) == LUA_TSTRING){
		const char * colname = lua_tostring(L, 3);
		for (int n = 0; n < twoda->numbcols; n++){
			if (strcmp(colname, twoda->columns[n]) == 0){
				col = n;
				break;
			}
		}
	}
	else{
		luaL_error(L, "get2dastring parameter 3 must be a number or string");
	}


	if (col < 0 || row < 0 || row >= twoda->numbrows){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	const char * data = twoda->rows[row][col];
	lua_pop(L, lua_gettop(L));
	if (!data)
		lua_pushstring(L, "");
	else
		lua_pushstring(L, data);

	return 1;
}

int twoda_get2darow(lua_State *L){

	Lua2da * twoda = lua_totwoda(L, 1);
	int index = luaL_checkinteger(L, 2);

	if (index < 0 || index >= twoda->numbrows){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	char ** row = twoda->rows[index];
	if (!row){
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	lua_pop(L, lua_gettop(L));
	lua_createtable(L, 0, twoda->numbcols);
	for (int n = 0; n < twoda->numbcols; n++)
	{
		lua_pushstring(L, twoda->columns[n]);
		if (row[n])
			lua_pushstring(L, row[n]);
		else
			lua_pushstring(L, "");
		lua_settable(L, -3);
	}

	return 1;
}

int twoda_get2dainfo(lua_State *L){

	Lua2da * twoda = lua_totwoda(L, 1);
	lua_pop(L, lua_gettop(L));
	lua_createtable(L, twoda->numbcols, 0);

	for (int n = 0; n < twoda->numbcols; n++){
		lua_pushstring(L, twoda->columns[n]);
		lua_rawseti(L, -2, n + 1);
	}

	lua_pushinteger(L, twoda->numbrows);
	lua_pushlstring(L, twoda->FileVersion, 4);

	return 3;
}

int twoda_gc(lua_State *L){

	Lua2da * tlk = lua_totwoda(L, 1);
	if (tlk->columns){
		for (int n = 0; n < tlk->numbcols; n++){
			if (tlk->columns[n])
				free(tlk->columns[n]);
		}
		free(tlk->columns);
		tlk->columns = NULL;
	}
	if (tlk->rows){
		for (int row = 0; row < tlk->numbrows; row++){
			if (tlk->rows[row])
			{
				for (int col = 0; col < tlk->numbcols; col++)
				{
					if (tlk->rows[row][col])
						free(tlk->rows[row][col]);
				}
				free(tlk->rows[row]);
			}
		}
		free(tlk->rows);
		tlk->rows = NULL;
	}
	return 0;
}

int twoda_tostring(lua_State *L){
	char tim[100];
	sprintf(tim, "2DA: 0x%08X", lua_totwoda(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}