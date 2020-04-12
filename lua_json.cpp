#include "lua_json.h"
#include "jsonutil.h"
#include "jsonencode.h"
#include "jsondecode.h"

int lua_jsondecodefunction(lua_State *L) {

	JsonContext * json = lua_tojson(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	lua_settop(L, 2);

	json_bail(L, json, NULL);
	json->refReadFunction = luaL_ref(L, LUA_REGISTRYINDEX);

	json_decodetable(L, json);
	json_bail(L, json, NULL);

	return 1;
}

int lua_jsonencodefunction(lua_State *L) {

	JsonContext * json = lua_tojson(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	luaL_checktype(L, 3, LUA_TTABLE);
	lua_settop(L, 3);

	json_bail(L, json, NULL);

	lua_pushvalue(L, 2);
	json->refWriteFunction = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_copy(L, 3, 2);
	lua_pop(L, 1);

	json_encodetable(L, json, NULL);
	json_append("", 0, L, json, true);
	json_bail(L, json, NULL);

	return 0;
}

int lua_jsondecodefromfile(lua_State *L) {

	JsonContext * json = lua_tojson(L, 1);
	size_t len;
	const char * file = luaL_checklstring(L, 2, &len);
	lua_settop(L, 2);

	json_bail(L, json, NULL);

	json->readFile = fopen(file, "r");

	if (!json->readFile) {
		luaL_error(L, "Failed to open file %s", file);
		return 0;
	}

	if (json->fileName) {
		free(json->fileName);
	}

	json->fileName = (char*)calloc(len + 1, sizeof(char));
	strcpy(json->fileName, file);

	json_decodetable(L, json);
	json_bail(L, json, NULL);

	return 1;
}

int lua_jsondecodestring(lua_State *L) {

	JsonContext * json = lua_tojson(L, 1);
	size_t len;
	const char * data = luaL_checklstring(L, 2, &len);
	lua_settop(L, 2);

	json_bail(L, json, NULL);

	json->read = data;
	json->readSize = len;

	json_decodetable(L, json);

	json_bail(L, json, NULL);

	return 1;
}

int lua_jsonencodetabletostring(lua_State *L) {

	JsonContext * json = lua_tojson(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);
	lua_settop(L, 2);

	json_encodetable(L, json, NULL);

	lua_pushlstring(L, json->buffer, json->bufferLength);
	json_bail(L, json, NULL);

	return 1;
}

int lua_jsonencodetabletofile(lua_State *L) {

	JsonContext * json = lua_tojson(L, 1);
	size_t filelen;
	const char * file = lua_tolstring(L, 2, &filelen);
	luaL_checktype(L, 3, LUA_TTABLE);
	lua_settop(L, 3);

	json->bufferFile = fopen(file, "w");
	if (!json->bufferFile) {
		luaL_error(L, "Failed to open file %s", file);
		return 0;
	}

	if (json->fileName) {
		free(json->fileName);
	}

	json->fileName = (char*)calloc(filelen+1, sizeof(char));
	strcpy(json->fileName, file);

	int depth = 0;
	json_encodetable(L, json, &depth);

	json_bail(L, json, NULL);

	return 0;
}

int lua_jsoncreate(lua_State *L) {

	JsonContext * json = lua_pushjson(L);
	return 1;
}

JsonContext * lua_pushjson(lua_State *L) {
	JsonContext * luajson = (JsonContext*)lua_newuserdata(L, sizeof(JsonContext));
	if (luajson == NULL)
		luaL_error(L, "Unable to push json");
	luaL_getmetatable(L, LUAJSON);
	lua_setmetatable(L, -2);
	memset(luajson, 0, sizeof(JsonContext));

	luajson->refWriteFunction = LUA_REFNIL;
	luajson->refReadFunction = LUA_REFNIL;

	return luajson;
}

JsonContext * lua_tojson(lua_State *L, int index) {
	JsonContext * luajson = (JsonContext*)luaL_checkudata(L, index, LUAJSON);
	if (luajson == NULL)
		luaL_error(L, "parameter is not a %s", LUAJSON);
	return luajson;
}

int json_gc(lua_State *L) {

	JsonContext * json = lua_tojson(L, 1);

	json_bail(L, json, NULL);

	return 0;
}

int json_tostring(lua_State *L) {
	char tim[100];
	sprintf(tim, "JSON: 0x%08X", lua_tojson(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}