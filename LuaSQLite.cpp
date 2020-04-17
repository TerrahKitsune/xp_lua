#include "LuaSQLite.h"
#include <string.h>
#include <stdlib.h>

LuaSQLite * luaL_checksqlite(lua_State *L, int index) {

	LuaSQLite * luasqlite = (LuaSQLite*)luaL_checkudata(L, index, LUASQLITE);
	if (luasqlite == NULL)
		luaL_error(L, "parameter is not a %s", LUASQLITE);
	return luasqlite;
}

LuaSQLite * lua_pushsqlite(lua_State *L) {

	LuaSQLite * luasqlite = (LuaSQLite*)lua_newuserdata(L, sizeof(LuaSQLite));
	if (luasqlite == NULL)
		luaL_error(L, "Unable to create sqlite connection");
	luaL_getmetatable(L, LUASQLITE);
	lua_setmetatable(L, -2);
	memset(luasqlite, 0, sizeof(LuaSQLite));
	return luasqlite;
}

void push_sqlitevalue(lua_State *L, sqlite3_stmt *pStmt, int idx) {
	switch (sqlite3_column_type(pStmt, idx)) {
	case SQLITE_INTEGER:
		lua_pushinteger(L, sqlite3_column_int64(pStmt, idx));
		break;
	case SQLITE_FLOAT:
		lua_pushnumber(L, sqlite3_column_double(pStmt, idx));
		break;
	case SQLITE_TEXT:
	case SQLITE_BLOB:
		lua_pushlstring(L, (const char*)sqlite3_column_blob(pStmt, idx), sqlite3_column_bytes(pStmt, idx));
		break;
	case SQLITE_NULL:
	default:
		lua_pushnil(L);
		break;
	}
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {

	lua_State *L = (lua_State *)NotUsed;

	lua_createtable(L, argc, 0);

	for (int n = 0; n < argc; n++) {
		lua_pushstring(L, azColName[n]);
		lua_pushstring(L, argv[n]);
		lua_settable(L, -3);
	}

	if (lua_pcall(L, 1, 0, NULL)) {
		return 1;
	}
	else {

		//Repush the function
		lua_pushvalue(L, -1);

		return 0;
	}
}

int SQLiteExecuteWithCallback(lua_State *L) {

	size_t len;
	LuaSQLite * luasqlite = (LuaSQLite*)luaL_checksqlite(L, 1);
	if (luasqlite->db == NULL)
		luaL_error(L, "SQLite instance has been closed");
	const char * query = luaL_checklstring(L, 2, &len);
	int err;
	char *zErrMsg = 0;
	char * current = (char*)gff_malloc(len + 1);
	if (current == NULL) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, "Unable to allocate memory for query");
		return 2;
	}

	current[len] = '\0';
	memcpy(current, query, len);

	if (lua_isfunction(L, 3)) {
		lua_pushvalue(L, 3);
		err = sqlite3_exec(luasqlite->db, query, callback, L, &zErrMsg);
	}
	else {
		err = sqlite3_exec(luasqlite->db, query, NULL, NULL, &zErrMsg);
	}

	if (err == SQLITE_ABORT) {
		lua_pushboolean(L, false);
		lua_pushfstring(L, "%s: %s", zErrMsg, lua_tostring(L, -2));
		gff_free(current);
		sqlite3_free(zErrMsg);
		return 2;
	}
	else if (err) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, true);
		lua_pushstring(L, "OK");
	}

	gff_free(current);
	return 2;
}

int SQLiteGetRow(lua_State *L) {

	LuaSQLite * luasqlite = (LuaSQLite*)luaL_checksqlite(L, 1);
	if (luasqlite->db == NULL)
		luaL_error(L, "SQLite instance has been closed");

	if (luasqlite->stmt == NULL) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		return 1;
	}
	else if (luasqlite->status != SQLITE_OK && luasqlite->status != SQLITE_ROW) {
		sqlite3_finalize(luasqlite->stmt);
		luasqlite->stmt = NULL;
		luasqlite->status = SQLITE_OK;
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
	}

	int idx = (int)luaL_optinteger(L, 2, -1);
	if (idx > 0) {
		idx--;
		if (idx >= sqlite3_column_count(luasqlite->stmt)) {
			lua_pop(L, lua_gettop(L));
			lua_pushnil(L);
		}
		else {
			lua_pop(L, lua_gettop(L));
			push_sqlitevalue(L, luasqlite->stmt, idx);
		}
		return 1;
	}

	int cnt = sqlite3_column_count(luasqlite->stmt);
	lua_pop(L, lua_gettop(L));
	lua_createtable(L, cnt, 0);
	for (int n = 0; n < cnt; n++) {

		lua_pushstring(L, sqlite3_column_name(luasqlite->stmt, n));
		push_sqlitevalue(L, luasqlite->stmt, n);
		lua_settable(L, -3);
	}

	return 1;
}

int SQLiteFetch(lua_State *L) {

	LuaSQLite * luasqlite = (LuaSQLite*)luaL_checksqlite(L, 1);
	if (luasqlite->db == NULL)
		luaL_error(L, "SQLite instance has been closed");

	if (luasqlite->stmt == NULL) {
		luasqlite->status = SQLITE_OK;
		lua_pop(L, 1);
		lua_pushboolean(L, false);
	}
	else if (luasqlite->status == SQLITE_ROW) {
		luasqlite->status = SQLITE_OK;
		lua_pop(L, 1);
		lua_pushboolean(L, true);
	}
	else if (luasqlite->status == SQLITE_OK) {

		if (sqlite3_step(luasqlite->stmt) == SQLITE_ROW) {
			lua_pop(L, 1);
			lua_pushboolean(L, true);
		}
		else {
			sqlite3_finalize(luasqlite->stmt);
			luasqlite->stmt = NULL;
			luasqlite->status = SQLITE_OK;
			lua_pop(L, 1);
			lua_pushboolean(L, false);
		}
	}
	else {
		sqlite3_finalize(luasqlite->stmt);
		luasqlite->stmt = NULL;
		luasqlite->status = SQLITE_OK;
		lua_pop(L, 1);
		lua_pushboolean(L, false);
	}

	return 1;
}

int SQLiteExecute(lua_State *L) {

	LuaSQLite * luasqlite = (LuaSQLite*)luaL_checksqlite(L, 1);
	if (luasqlite->db == NULL)
		luaL_error(L, "SQLite instance has been closed");
	const char * query = luaL_checkstring(L, 2);
	int cnt;
	size_t len;
	const char * data;
	const char * name;

	if (luasqlite->stmt) {
		sqlite3_finalize(luasqlite->stmt);
		luasqlite->stmt = NULL;
	}

	int err = sqlite3_prepare_v2(luasqlite->db, query, -1, &luasqlite->stmt, 0);
	if (err) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);
		lua_pushstring(L, sqlite3_errmsg(luasqlite->db));
		return 2;
	}
	else if (lua_istable(L, 3)) {
		cnt = 0;

		for (int n = 0; n < sqlite3_bind_parameter_count(luasqlite->stmt); n++) {
			name = sqlite3_bind_parameter_name(luasqlite->stmt, n + 1);
			if (name == NULL || strlen(name) < 2) {
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				lua_pushstring(L, "Parameters contain a nameless parameter!");
				return 2;
			}

			lua_pushstring(L, &name[1]);
			lua_gettable(L, -2);

			switch (lua_type(L, -1))
			{
			case LUA_TNIL:
				sqlite3_bind_null(luasqlite->stmt, ++cnt);
				break;
			case LUA_TNUMBER:
				sqlite3_bind_double(luasqlite->stmt, ++cnt, lua_tonumber(L, -1));
				break;
			case LUA_TBOOLEAN:
				sqlite3_bind_int(luasqlite->stmt, ++cnt, lua_toboolean(L, -1));
				break;
			case LUA_TSTRING:
				data = lua_tolstring(L, -1, &len);
				sqlite3_bind_blob64(luasqlite->stmt, ++cnt, data, len, NULL);
				break;
			}

			lua_pop(L, 1);
		}
	}

	luasqlite->status = sqlite3_step(luasqlite->stmt);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);
	lua_pushstring(L, "OK");
	return 2;
}

static void RemoveBusyHandler(lua_State *L, LuaSQLite * luasqlite) {
	if (luasqlite->busyhandler != -1) {
		luaL_unref(L, LUA_REGISTRYINDEX, luasqlite->busyhandler);
		luasqlite->busyhandler = -1;
		sqlite3_busy_handler(luasqlite->db, NULL, luasqlite);
	}
}

static int BusyHandler(void * d, int retries) {
	lua_State *L = (lua_State*)d;

	LuaSQLite * luasqlite = (LuaSQLite*)luaL_checksqlite(L, 1);
	lua_rawgeti(L, LUA_REGISTRYINDEX, luasqlite->busyhandler);
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 1);
		return 0;
	}
	else {
		lua_pushvalue(L, 1);
		lua_pushinteger(L, retries);
		if (lua_pcall(L, 2, 1, NULL)) {
			return 0;
		}		
		bool ok = lua_toboolean(L, -1)>0;
		lua_pop(L, 1);
		return 1;
	}	
	return 0;
}

int SQLiteSetBusyHandler(lua_State *L) {

	LuaSQLite * luasqlite = (LuaSQLite*)luaL_checksqlite(L, 1);
	if (luasqlite->db == NULL)
		luaL_error(L, "SQLite instance has been closed");

	RemoveBusyHandler(L, luasqlite);

	if (lua_type(L, 2) == LUA_TFUNCTION)
	{
		lua_pushvalue(L, 2);
		luasqlite->busyhandler = luaL_ref(L, LUA_REGISTRYINDEX);
		sqlite3_busy_handler(luasqlite->db, BusyHandler, L);
	}

	return 0;
}

int SQLiteConnect(lua_State *L) {

	size_t len;
	const char * db = luaL_optlstring(L, 1, ":memory:", &len);
	char * file = (char*)gff_malloc(len + 1);
	if (!file)
		luaL_error(L, "Unable to allocate memory for sqlite");
	file[len] = '\0';
	memcpy(file, db, len);

	int mode = (int)luaL_optinteger(L, 2, 0);

	lua_pop(L, lua_gettop(L));
	LuaSQLite * luasqlite = lua_pushsqlite(L);
	if (!luasqlite) {
		gff_free(file);
		luaL_error(L, "Unable to allocate memory for sqlite");
	}
	luasqlite->busyhandler = -1;

	int ok;

	switch (mode) {
	case 1:
		ok = sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
		break;
	case 2:
		ok = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
		break;
	default:
		ok = sqlite3_config(SQLITE_CONFIG_SINGLETHREAD);
		break;
	}

	//Ignore missuse
	if (ok != SQLITE_OK && ok != SQLITE_MISUSE) {
		gff_free(file);
		luaL_error(L, "SQLite error %s", sqlite3_errmsg(luasqlite->db));
	}

	int err = sqlite3_open(file, &luasqlite->db);
	if (err) {
		gff_free(file);
		luaL_error(L, "SQLite error %s", sqlite3_errmsg(luasqlite->db));
	}

	if (mode != 0) {
		sqlite3_exec(luasqlite->db, "PRAGMA journal_mode=WAL;", 0, 0, 0);
	}
	else {
		sqlite3_exec(luasqlite->db, "PRAGMA journal_mode=DELETE;", 0, 0, 0);
	}

	sqlite3_exec(luasqlite->db, "PRAGMA synchronous=NORMAL;", 0, 0, 0);

	luasqlite->file = file;

	return 1;
}

int SQLite_GC(lua_State *L) {

	LuaSQLite * luasqlite = (LuaSQLite*)luaL_checksqlite(L, 1);

	RemoveBusyHandler(L, luasqlite);

	if (luasqlite->stmt) {
		sqlite3_finalize(luasqlite->stmt);
		luasqlite->stmt = NULL;
	}

	if (luasqlite->db) {
		sqlite3_close(luasqlite->db);
		luasqlite->db = NULL;
	}

	if (luasqlite->file) {
		gff_free(luasqlite->file);
		luasqlite->file = NULL;
	}

	return 0;
}

int SQLite_ToString(lua_State *L) {

	LuaSQLite * sq = luaL_checksqlite(L, 1);
	char sqlite[_MAX_PATH + 20];
	sprintf(sqlite, "SQLite: 0x%08X File: %s", sq, sq->file);

	lua_pushstring(L, sqlite);
	return 1;
}