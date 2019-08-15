#include "ResourcesTypes.h"
#include <string>

int lua_pushresourcelist(lua_State * L){

	int count = 0;
	res_list_entry entry = resourcetypes[count];

	while (entry.key != 0xFFFF){
		entry = resourcetypes[++count];
	}

	lua_createtable(L, 0, 3);

	for (int n = 0; n < count; n++)
	{
		lua_pushinteger(L, resourcetypes[n].key);
		lua_pushstring(L, resourcetypes[n].value);
		lua_settable(L, -3);
	}

	lua_setglobal(L, RESOURCELISTKEY);

	return 0;
}

void pushvalidate(lua_State * L){

	lua_getglobal(L, RESOURCELISTKEY);

	if (!lua_istable(L, -1)){
		lua_pop(L, 1);
		lua_pushresourcelist(L);
		lua_getglobal(L, RESOURCELISTKEY);
	}
}

const char * lua_resource_getextension(lua_State * L, int key, const char * extdefault){

	const char * result = extdefault;

	pushvalidate(L);

	lua_pushinteger(L, key);
	lua_gettable(L, -2);

	if (lua_isstring(L, -1))
		result = lua_tostring(L, -1);

	lua_pop(L, 2);

	return result;
}

int lua_resource_getresourceid(lua_State * L, const char * file, size_t len){

	int result = 0xFFFF;

	if (!file)
		return result;
	if (len <= 0)
		len = strlen(file);

	const char * start = file;
	size_t remainderlength = len;

	for (size_t n = 0; n < len; n++){
		if (file[n] == '.' && n + 1 < len){
			start = &file[n + 1];
			remainderlength = len - (n + 1);
		}
	}

	char * ext = (char*)calloc(remainderlength + 1, sizeof(char));
	if (!ext)
		return result;
	else
		memcpy(ext, start, remainderlength);

	pushvalidate(L);

	lua_pushnil(L);

	while (lua_next(L, -2) != 0) {

		if (lua_isstring(L, -1) && lua_isnumber(L, -2) && _stricmp(ext, lua_tostring(L, -1)) == 0){
			result = (int)lua_tointeger(L, -2);
			//pop key and value and end iteration
			lua_pop(L, 2);
			break;
		}

		//pop value
		lua_pop(L, 1);
	}

	free(ext);

	lua_pop(L, 1);

	return result;
}