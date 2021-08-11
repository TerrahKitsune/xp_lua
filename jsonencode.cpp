#include "jsonencode.h"
#include "jsonutil.h"
#include "math.h"

void json_encodenumber(lua_State *L, JsonContext* context) {

	double numb = lua_tonumber(L, -1);
	char number[50];

	if (isfinite(numb)) {

		long long rounded = llround(numb);

		if (rounded == numb) {
			sprintf(number, "%lld", rounded);
		}
		else {
			sprintf(number, "%.16f", numb);
			for (size_t n = strlen(number) - 1; n > 0; n++) {
				if (number[n] == '0') {
					number[n] = '\0';
				}
				else if (number[n] == '.') {
					number[n + 1] = '0';
					break;
				}
				else {
					break;
				}
			}
		}
	}
	else if (isnan(numb)) {
		strcpy(number, "null");
	}
	else {

		if (numb == INFINITY) {
			strcpy(number, "1e+9999");
		}
		else if (numb == -INFINITY) {
			strcpy(number, "-1e+9999");
		}
		else {
			strcpy(number, "null");
		}
	}

	json_append(number, strlen(number), L, context);
}

void json_encodevalue(lua_State *L, JsonContext* context, int* depth) {

	if (json_isnull(L, context)) {

		json_append("null", 4, L, context);
		return;
	}

	switch (lua_type(L, -1)) {
	case LUA_TTABLE:
		json_encodetable(L, context, depth);
		break;
	case LUA_TNUMBER:
		json_encodenumber(L, context);
		break;
	case LUA_TBOOLEAN:
		json_append(lua_toboolean(L, -1) > 0 ? "true" : "false", lua_toboolean(L, -1) > 0 ? 4 : 5, L, context);
		break;
	default:
		if (lua_isnoneornil(L, -1)) {
			json_append("null", 4, L, context);
		}
		else {
			json_encodestring(L, context);
		}
		break;
	}
}

void json_encodestring(lua_State* L, JsonContext* C) {

	size_t stackSize = lua_gettop(L);
	size_t len;
	const char * str = luaL_tolstring(L, -1, &len);
	char hex[7] = { 0 };
	int data = 0;

	json_append("\"", 1, L, C);
	for (size_t i = 0; i < len; i++)
	{
		switch (str[i]) {
		case '\"':
			json_append("\\\"", 2, L, C);
			break;
		case '\\':
			json_append("\\\\", 2, L, C);
			break;
		case '/':
			json_append("\\/", 2, L, C);
			break;
		case '\n':
			json_append("\\n", 2, L, C);
			break;
		case '\r':
			json_append("\\r", 2, L, C);
			break;
		default:

			if (str[i] < 32 || str[1] > 127) {
				memcpy(&data, &str[i], 1);
				sprintf(hex, "\\u%04x", data);
				json_append(hex, 6, L, C);
			}
			else {
				json_append(&str[i], 1, L, C);
			}

			break;
		}
	}

	json_append("\"", 1, L, C);

	if (lua_gettop(L) != stackSize) {
		lua_settop(L, stackSize);
	}
}

void json_pad(char padding, int numbpadding, lua_State*L, JsonContext*C) {

	if (numbpadding <= 0) {
		return;
	}

	for (int i = 0; i < numbpadding; i++)
	{
		json_append("\t", 1, L, C);
	}
}

void json_encodetable(lua_State* L, JsonContext* C, int* depth) {

	if (C->refThreadInput != LUA_REFNIL) {
		json_encodethread(L, C, depth);
		return;
	}

	size_t len;
	const char * rawid = luaL_tolstring(L, -1, &len);
	lua_pop(L, 1);
	lua_len(L, -1);
	int size = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	unsigned int id = table_crc32((const unsigned char*)rawid, len);

	if (json_existsinantirecursion(id, C)) {
		json_bail(L, C, "Recursion detected");
		return;
	}
	else if (!json_addtoantirecursion(id, C)) {

		lua_gc(L, LUA_GCCOLLECT, 0);

		if (!json_addtoantirecursion(id, C)) {
			json_bail(L, C, "Out of memory");
			return;
		}
	}

	size_t pos = 0;
	int objlen = 0;

	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		objlen++;

		lua_pop(L, 1);
	}

	int count = 0;

	// Object
	if (objlen == size && size <= 0) {
		json_append("[]", 2, L, C);
	}
	else if (objlen != size) {

		json_append("{", 1, L, C);

		if (depth) {
			(*depth)++;
			json_append("\n", 1, L, C);
		}

		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {

			if (depth) {
				json_pad('\t', *depth, L, C);
			}

			lua_pushvalue(L, -2);
			json_encodestring(L, C);
			lua_pop(L, 1);
			if (depth) {
				json_append(": ", 2, L, C);
			}
			else {
				json_append(":", 1, L, C);
			}
			json_encodevalue(L, C, depth);

			if (++count < objlen) {
				if (depth) {
					json_append(",\n", 2, L, C);
				}
				else {
					json_append(",", 1, L, C);
				}
			}

			lua_pop(L, 1);
		}

		if (depth) {
			(*depth)--;
			json_append("\n", 1, L, C);
			json_pad('\t', *depth, L, C);
			json_append("}", 1, L, C);
		}
		else {
			json_append("}", 1, L, C);
		}
	}
	// Array
	else {

		json_append("[", 1, L, C);

		if (depth) {
			(*depth)++;
			json_append("\n", 1, L, C);
		}

		for (int i = 1; i <= size; i++)
		{
			if (depth) {
				json_pad('\t', *depth, L, C);
			}

			lua_rawgeti(L, -1, i);
			json_encodevalue(L, C, depth);
			if (++count < objlen) {
				if (depth) {
					json_append(",\n", 2, L, C);
				}
				else {
					json_append(",", 1, L, C);
				}
			}

			lua_pop(L, 1);
		}

		if (depth) {
			(*depth)--;
			json_append("\n", 1, L, C);
			json_pad('\t', *depth, L, C);
			json_append("]", 1, L, C);
		}
		else {
			json_append("]", 1, L, C);
		}
	}

	json_removefromantirecursion(id, C);
}

void json_getnextthread(lua_State* L, JsonContext* C) {

	lua_rawgeti(L, LUA_REGISTRYINDEX, C->refThreadInput);

	lua_State*T = lua_tothread(L, -1);
	int result = lua_resume(T, L, 0);

	if (result == LUA_YIELD) {
		lua_pop(L, 1);
		lua_xmove(T, L, 2);
		lua_pop(T, lua_gettop(T));
	}
	else if (result == 0) {
		lua_pop(L, 1);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	else {
		lua_xmove(T, L, 1);
		const char * err = lua_tostring(L, -1);
		if (!err) {
			err = "Coroutine error";
		}
		json_bail(L, C, err);
	}
}

void json_encodethread(lua_State* L, JsonContext* C, int* depth) {

	char firstType = '\0';

	int count = 0;
	json_getnextthread(L, C);
	while (!lua_isnil(L, -1) || !lua_isnil(L, -2)) {

		if (firstType == '\0') {

			if (lua_type(L, -2) == LUA_TSTRING) {
				firstType = '{';
			}
			else {
				firstType = '[';
			}

			json_append(&firstType, 1, L, C);

			if (depth) {
				(*depth)++;
				json_append("\n", 1, L, C);
			}

			firstType = firstType == '{' ? '}' : ']';
		}

		if (firstType == ']') {

			if (count++ > 0) {
				if (depth) {
					json_append(",\n", 2, L, C);
				}
				else {
					json_append(",", 1, L, C);
				}
			}

			if (depth) {
				json_pad('\t', *depth, L, C);
			}

			json_encodevalue(L, C, depth);
		}
		else {

			if (count++ > 0) {
				if (depth) {
					json_append(",\n", 2, L, C);
				}
				else {
					json_append(",", 1, L, C);
				}
			}

			if (depth) {
				json_pad('\t', *depth, L, C);
			}

			lua_pushvalue(L, -2);
			json_encodestring(L, C);
			lua_pop(L, 1);
			if (depth) {
				json_append(": ", 2, L, C);
			}
			else {
				json_append(":", 1, L, C);
			}

			json_encodevalue(L, C, depth);
		}

		lua_pop(L, 2);
		json_getnextthread(L, C);
	}

	lua_pop(L, 2);

	if (firstType == '\0') {
		json_append("[]", 2, L, C);
		return;
	}

	if (depth) {
		(*depth)--;
		json_append("\n", 1, L, C);
		json_pad('\t', *depth, L, C);
		json_append(&firstType, 1, L, C);
	}
	else {
		json_append(&firstType, 1, L, C);
	}
}