#include "2da.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

Lua2daLine GetRow(const char * original, const char * end)
{
	Lua2daLine line;
	memset(&line, 0, sizeof(Lua2daLine));

	if (original >= end)
		return line;

	for (const char * c = original; c < end; c++) {

		if (*c == '\n') {

			line.start = original;
			line.len = c - original;
			break;
		}
	}

	if (!line.start) {
		line.start = original;
		line.len = end - original;
	}

	return line;
}

Lua2daLine GetField(const char * original, const char * end)
{
	Lua2daLine line;
	memset(&line, 0, sizeof(Lua2daLine));

	if (original >= end)
		return line;

	const char * start = NULL;
	bool isinescape = false;

	for (const char * c = original; c < end; c++) {

		if (start) {

			if (isinescape) {

				if (*c == '"') {
					line.start = start;
					line.len = c - start;
					break;
				}
			}
			else if (*c <= ' ')
			{
				line.start = start;
				line.len = c - start;
				break;
			}
		}
		else {
			if (*c != ' ' && *c != '\t' && *c != '\r') {
				start = c;

				if (*c == '"') {
					isinescape = true;
					start++;
				}
			}
		}
	}

	return line;
}

void AddToList(Linked * root, void * data) {

	Linked * newlinked = NULL;

	if (root->data == NULL) {
		root->data = data;
		return;
	}

	newlinked = (Linked *)gff_calloc(1, sizeof(Linked));

	if (root->next == NULL) {
		root->next = newlinked;
	}
	else {
		root->bottom->next = newlinked;
	}

	if (newlinked) {
		newlinked->data = data;
		root->bottom = newlinked;
	}
}

void freelinked(Linked * root) {
	Linked * l = root;
	Linked * temp;
	while (l) {
		temp = l;
		l = l->next;
		gff_free(temp);
	}
}

int twoda_open(lua_State *L) {

	size_t len;
	const char * raw = luaL_checklstring(L, 1, &len);
	Lua2da header;

	memset(&header, 0, sizeof(Lua2da));

	if (len < sizeof(Lua2da)) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Data length to short");
		return 2;
	}

	memcpy(&header, raw, 8);

	if (header.FileType[0] != '2' ||
		header.FileType[1] != 'D' ||
		header.FileType[2] != 'A' ||
		header.FileVersion[0] != 'V' ||
		header.FileVersion[1] != '2' ||
		header.FileVersion[2] != '.' ||
		header.FileVersion[3] != '0')
	{
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Not a 2da file");
		return 2;
	}

	const char * end = &raw[len];
	const char * cursor;

	Lua2daLine first = GetRow(raw, end);
	Lua2daLine second = GetRow(&first.start[first.len] + 1, end);

	if (!first.start ||
		!second.start)
	{
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "2da is malformed");
		return 2;
	}
	else {

		if (!second.start) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "2da is malformed");
			return 2;
		}

		cursor = &second.start[second.len] + 1;
	}

	Lua2daLine columns = GetRow(cursor, end);
	const char * lineend = &columns.start[columns.len + 1];
	Lua2daLine field = GetField(columns.start, lineend);
	char * fielddata;
	Linked * list = (Linked*)gff_calloc(1, sizeof(Linked));

	while (field.start)
	{
		fielddata = (char*)gff_calloc(field.len + 1, sizeof(char));
		memcpy(fielddata, field.start, field.len);

		AddToList(list, fielddata);

		header.numbcols++;
		field = GetField(&field.start[field.len + 1], lineend);
	}

	if (header.numbcols <= 0) {
		freelinked(list);
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "No columns found");
		return 2;
	}

	int cnt = 0;

	header.columns = (char**)gff_calloc(header.numbcols, sizeof(char*));
	for (Linked * l = list; l; l = l->next) {
		header.columns[cnt++] = (char*)l->data;
	}

	freelinked(list);
	list = (Linked*)gff_calloc(1, sizeof(Linked));

	Linked * sub;
	Lua2daLine line = GetRow(&columns.start[columns.len + 1], end);

	while (line.start) {

		sub = (Linked*)gff_calloc(1, sizeof(Linked));
		lineend = &line.start[line.len + 1];

		field = GetField(line.start, lineend);

		while (field.start)
		{
			fielddata = (char*)gff_calloc(field.len + 1, sizeof(char));
			memcpy(fielddata, field.start, field.len);

			AddToList(sub, fielddata);

			field = GetField(&field.start[field.len + 1], lineend);
		}

		AddToList(list, sub);

		header.numbrows++;

		line = GetRow(lineend, end);
	}

	cnt = 0;
	int subcnt;
	if (header.numbrows > 0) {
		header.rows = (char***)gff_calloc(header.numbrows, sizeof(char**));
		for (Linked * l = list; l; l = l->next) {

			sub = (Linked*)l->data;

			subcnt = 0;

			header.rows[cnt] = (char**)gff_calloc(header.numbcols, sizeof(char*));

			if (sub) {

				for (Linked * s = sub->next; s; s = s->next) {
					header.rows[cnt][subcnt] = (char*)s->data;
					if (++subcnt >= header.numbcols)
						break;
				}
				freelinked(sub);
			}

			cnt++;
		}
	}
	else {
		header.rows = NULL;
	}

	freelinked(list);

	lua_pop(L, lua_gettop(L));
	Lua2da * data = lua_pushtwoda(L);
	memcpy(data, &header, sizeof(Lua2da));

	return 1;
}

Lua2da * lua_pushtwoda(lua_State *L) {
	Lua2da * twoda = (Lua2da*)lua_newuserdata(L, sizeof(Lua2da));
	if (twoda == NULL)
		luaL_error(L, "Unable to push 2da");
	luaL_getmetatable(L, TWODA);
	lua_setmetatable(L, -2);
	memset(twoda, 0, sizeof(Lua2da));
	return twoda;
}

Lua2da * lua_totwoda(lua_State *L, int index) {
	Lua2da * tlk = (Lua2da*)luaL_checkudata(L, index, TWODA);
	if (tlk == NULL)
		luaL_error(L, "parameter is not a %s", TWODA);
	return tlk;
}

int twoda_get2dastring(lua_State *L) {

	Lua2da * twoda = lua_totwoda(L, 1);
	int row = (int)luaL_checkinteger(L, 2);
	int col = -1;

	if (lua_type(L, 3) == LUA_TNUMBER) {
		col = (int)lua_tointeger(L, 3);
	}
	else if (lua_type(L, 3) == LUA_TSTRING) {
		const char * colname = lua_tostring(L, 3);
		for (int n = 0; n < twoda->numbcols; n++) {
			if (strcmp(colname, twoda->columns[n]) == 0) {
				col = n;
				break;
			}
		}
	}
	else {
		luaL_error(L, "get2dastring parameter 3 must be a number or string");
	}


	if (col < 0 || row < 0 || row >= twoda->numbrows) {
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

int twoda_get2darow(lua_State *L) {

	Lua2da * twoda = lua_totwoda(L, 1);
	int index = (int)luaL_checkinteger(L, 2);

	if (index < 0 || index >= twoda->numbrows) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}

	char ** row = twoda->rows[index];
	if (!row) {
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

int twoda_get2dainfo(lua_State *L) {

	Lua2da * twoda = lua_totwoda(L, 1);
	lua_pop(L, lua_gettop(L));
	lua_createtable(L, twoda->numbcols, 0);

	for (int n = 0; n < twoda->numbcols; n++) {
		lua_pushstring(L, twoda->columns[n]);
		lua_rawseti(L, -2, n + 1);
	}

	lua_pushinteger(L, twoda->numbrows);
	lua_pushlstring(L, twoda->FileVersion, 4);

	return 3;
}

void freelua2dasubdata(Lua2da * data) {

	if (data->columns) {
		for (int n = 0; n < data->numbcols; n++) {
			if (data->columns[n])
				gff_free(data->columns[n]);
		}
		gff_free(data->columns);
		data->columns = NULL;
	}

	if (data->rows) {
		for (int row = 0; row < data->numbrows; row++) {
			if (data->rows[row])
			{
				for (int col = 0; col < data->numbcols; col++)
				{
					if (data->rows[row][col])
						gff_free(data->rows[row][col]);
				}
				gff_free(data->rows[row]);
			}
		}
		gff_free(data->rows);
		data->rows = NULL;
	}
}

int twoda_gc(lua_State *L) {

	Lua2da * tlk = lua_totwoda(L, 1);
	freelua2dasubdata(tlk);
	return 0;
}

int twoda_tostring(lua_State *L) {
	char tim[100];
	sprintf(tim, "2DA: 0x%08X", lua_totwoda(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}