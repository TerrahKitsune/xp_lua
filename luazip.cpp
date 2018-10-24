#include "luazip.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 

int zip_deletefile(lua_State *L) {

	LuaZIP * zip = lua_tozip(L, 1);
	if (!zip || !zip->z) {
		luaL_error(L, "zip file is null");
		return 0;
	}

	struct zip_stat sb;

	if (lua_type(L, 2) == LUA_TNUMBER) {

		if (zip_stat_index(zip->z, lua_tointeger(L, 2), 0, &sb) != 0) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Unable to retrive entry");
			return 2;
		}
	}
	else {
		const char * key = lua_tostring(L, 2);

		if (!key) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Supplied key is invalid");
			return 2;
		}

		zip_stat_init(&sb);

		if (zip_stat(zip->z, key, 0, &sb) != 0) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Unable to retrive entry");
			return 2;
		}
	}

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, zip_delete(zip->z, sb.index));

	return 1;
}

int zip_addfile(lua_State *L) {

	LuaZIP * zip = lua_tozip(L, 1);
	if (!zip || !zip->z) {
		luaL_error(L, "zip file is null");
		return 0;
	}

	const char * key = luaL_checkstring(L, 2);
	const char * file = luaL_checkstring(L, 3);

	zip_source *source = zip_source_file(zip->z, file, 0, 0);

	if (!source) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, zip_strerror(zip->z));
		return 2;
	}

	int index = (int)zip_file_add(zip->z, key, source, ZIP_FL_OVERWRITE);
	if (index < 0)
	{
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, zip_strerror(zip->z));
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, index);

	return 1;
}

int zip_addbuffer(lua_State *L) {

	LuaZIP * zip = lua_tozip(L, 1);
	if (!zip || !zip->z) {
		luaL_error(L, "zip file is null");
		return 0;
	}

	const char * key = luaL_checkstring(L, 2);
	size_t len;
	const char * data = lua_tolstring(L, 3, &len);

	if (!data || !key) {

		luaL_error(L, "Invalid parameters supplied to zip addfile");
		return 0;
	}

	zip_source *source = zip_source_buffer(zip->z, data, len, 0);

	if (!source) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, zip_strerror(zip->z));
		return 2;
	}

	int index = (int)zip_file_add(zip->z, key, source, ZIP_FL_OVERWRITE);

	if (index < 0)
	{
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, zip_strerror(zip->z));
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, index);

	return 1;
}

int zip_extract(lua_State *L) {

	LuaZIP * zip = lua_tozip(L, 1);
	if (!zip || !zip->z) {
		luaL_error(L, "zip file is null");
		return 0;
	}

	struct zip_stat sb;

	if (lua_type(L, 2) == LUA_TNUMBER) {

		if (zip_stat_index(zip->z, lua_tointeger(L, 2), 0, &sb) != 0) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Unable to retrive entry");
			return 2;
		}
	}
	else {
		const char * key = lua_tostring(L, 2);

		if (!key) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Supplied key is invalid");
			return 2;
		}

		zip_stat_init(&sb);

		if (zip_stat(zip->z, key, 0, &sb) != 0) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Unable to retrive entry");
			return 2;
		}
	}

	struct zip_file *zf = zip_fopen_index(zip->z, sb.index, 0);

	if (!zf) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Unable to open zip record");
		return 2;
	}

	if (lua_type(L, 3) == LUA_TSTRING) {

		FILE * file = fopen(lua_tostring(L, 3), "r");
		if (file) {

			zip_fclose(zf);
			fclose(file);
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "File already exists");
			return 2;
		}

		file = fopen(lua_tostring(L, 3), "wb");
		if (!file) {

			zip_fclose(zf);
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Unable to create file");
			return 2;
		}

		char buf[1000];

		zip_int64_t len;
		zip_int64_t sum = 0;

		while (sum < sb.size) {

			len = zip_fread(zf, buf, 1000);

			if (len < 0) {

				zip_fclose(zf);
				fclose(file);
				remove(lua_tostring(L, 3));

				lua_pop(L, lua_gettop(L));
				lua_pushnil(L);
				lua_pushstring(L, "Unable to read zip entry");
				return 2;
			}

			fwrite(buf, 1, len, file);
			fflush(file);
			sum += len;
		}

		fflush(file);
		fclose(file);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, true);
	}
	else {

		char * buffer = (char*)malloc(sb.size);
		if (!buffer) {

			zip_fclose(zf);

			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Unable to allocate memory");
			return 2;
		}

		if (zip_fread(zf, buffer, sb.size) != sb.size) {

			free(buffer);
			zip_fclose(zf);

			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Unable to read contents");
			return 2;
		}

		lua_pop(L, lua_gettop(L));
		lua_pushlstring(L, buffer, sb.size);
		free(buffer);
	}

	zip_fclose(zf);

	return 1;
}

int zip_getinfo(lua_State *L) {

	LuaZIP * zip = lua_tozip(L, 1);
	if (!zip || !zip->z) {
		luaL_error(L, "zip file is null");
		return 0;
	}

	struct zip_stat sb;

	if (lua_type(L, 2) == LUA_TNUMBER) {

		if (zip_stat_index(zip->z, lua_tointeger(L, 2), 0, &sb) != 0) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Unable to retrive entry");
			return 2;
		}
	}
	else {
		const char * key = lua_tostring(L, 2);

		if (!key) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Supplied key is invalid");
			return 2;
		}

		zip_stat_init(&sb);

		if (zip_stat(zip->z, key, 0, &sb) != 0) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Unable to retrive entry");
			return 2;
		}
	}

	lua_pop(L, lua_gettop(L));

	lua_createtable(L, 0, 10);

	lua_pushstring(L, "comp_method");
	lua_pushinteger(L, sb.comp_method);
	lua_settable(L, -3);

	lua_pushstring(L, "comp_size");
	lua_pushinteger(L, sb.comp_size);
	lua_settable(L, -3);

	lua_pushstring(L, "crc");
	lua_pushinteger(L, sb.crc);
	lua_settable(L, -3);

	lua_pushstring(L, "encryption_method");
	lua_pushinteger(L, sb.encryption_method);
	lua_settable(L, -3);

	lua_pushstring(L, "flags");
	lua_pushinteger(L, sb.flags);
	lua_settable(L, -3);

	lua_pushstring(L, "index");
	lua_pushinteger(L, sb.index);
	lua_settable(L, -3);

	lua_pushstring(L, "mtime");
	lua_pushinteger(L, sb.mtime);
	lua_settable(L, -3);

	lua_pushstring(L, "name");
	lua_pushstring(L, sb.name);
	lua_settable(L, -3);

	lua_pushstring(L, "size");
	lua_pushinteger(L, sb.size);
	lua_settable(L, -3);

	lua_pushstring(L, "valid");
	lua_pushinteger(L, sb.valid);
	lua_settable(L, -3);

	return 1;
}

int zip_getfiles(lua_State *L) {

	LuaZIP * zip = lua_tozip(L, 1);
	if (!zip || !zip->z) {
		luaL_error(L, "zip file is null");
		return 0;
	}

	struct zip_stat sb;
	zip_int64_t entries = zip_get_num_entries(zip->z, 0);

	lua_pop(L, lua_gettop(L));
	lua_createtable(L, entries, 0);

	for (int n = 0; n < entries; n++) {
		if (zip_stat_index(zip->z, n, 0, &sb) != 0) {

			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
			lua_pushstring(L, "Unable to retrive entries");
			return 2;
		}

		if (sb.valid & ZIP_STAT_NAME && sb.name) {
			lua_pushstring(L, sb.name);
		}
		else {
			lua_pushstring(L, "");
		}

		lua_rawseti(L, -2, n + 1);
	}

	return 1;
}

int zip_open(lua_State *L) {

	const char * file = luaL_checkstring(L, 1);

	int err = 0;
	zip *z = zip_open(file, ZIP_CREATE, &err);

	lua_pop(L, lua_gettop(L));

	if (!z || err > 0) {

		lua_pushnil(L);
		lua_pushfstring(L, "Unable to open zipfile (%d)", err);
		return 2;
	}

	LuaZIP * zip = lua_pushzip(L);

	zip->z = z;

	return 1;
}

LuaZIP * lua_pushzip(lua_State *L) {
	LuaZIP * zip = (LuaZIP*)lua_newuserdata(L, sizeof(LuaZIP));
	if (zip == NULL)
		luaL_error(L, "Unable to push zip");
	luaL_getmetatable(L, ZIP);
	lua_setmetatable(L, -2);
	memset(zip, 0, sizeof(LuaZIP));
	return zip;
}

LuaZIP * lua_tozip(lua_State *L, int index) {
	LuaZIP * tlk = (LuaZIP*)luaL_checkudata(L, index, ZIP);
	if (tlk == NULL)
		luaL_error(L, "parameter is not a %s", ZIP);
	return tlk;
}

int zip_close(lua_State *L) {

	LuaZIP * zip = lua_tozip(L, 1);
	if (!zip || !zip->z) {
		luaL_error(L, "zip file is null");
		return 0;
	}

	if (zip_close(zip->z) == -1) {

		lua_settop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, zip_strerror(zip->z));
		return 2;
	}

	zip->z = NULL;
	lua_settop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int zip_gc(lua_State *L) {

	LuaZIP * zip = lua_tozip(L, 1);

	if (zip && zip->z) {

		zip_close(zip->z);
		zip->z = NULL;
	}

	return 0;
}

int zip_tostring(lua_State *L) {
	char tim[100];
	sprintf(tim, "Zip: 0x%08X", lua_tozip(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}