#include "LuaMySQLResult.h"

int MySQLResultSetAsString(lua_State *L) {
	LuaMySQLResult * luamysql = luaL_checkmysqlresult(L, 1);
	luamysql->asstring = lua_toboolean(L, 2);
	lua_pop(L, lua_gettop(L));
	return 0;
}

int MySQLResultNumbRows(lua_State *L) {

	LuaMySQLResult * luamysql = luaL_checkmysqlresult(L, 1);

	lua_pop(L, 1);
	lua_pushinteger(L, luamysql->rows);
	return 1;
}

int MySQLResultFetch(lua_State *L) {

	LuaMySQLResult * luamysql = luaL_checkmysqlresult(L, 1);

	if (luamysql->result == NULL) {
		lua_pop(L, 1);
		lua_pushboolean(L, false);
		return 1;
	}

	if (!luamysql->fields)
		luamysql->fields = mysql_num_fields(luamysql->result);
	if (!luamysql->columns)
		luamysql->columns = mysql_fetch_fields(luamysql->result);

	luamysql->row = mysql_fetch_row(luamysql->result);
	if (luamysql->row == NULL) {

		mysql_free_result(luamysql->result);
		luamysql->result = NULL;
		luamysql->row = NULL;
		luamysql->columns = NULL;
		luamysql->fields = NULL;

		lua_pop(L, 1);
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pop(L, 1);
	lua_pushboolean(L, true);
	return 1;
}

int MySQLResultGetRow(lua_State *L) {

	LuaMySQLResult * luamysql = luaL_checkmysqlresult(L, 1);

	unsigned long *lengths;
	int index = luaL_optinteger(L, 2, -1);

	lua_pop(L, lua_gettop(L));

	if (luamysql->row == NULL || luamysql->result == NULL || luamysql->columns == NULL) {
		lua_pushnil(L);
		return 1;
	}

	lengths = mysql_fetch_lengths(luamysql->result);

	if (lengths == NULL) {
		lua_pushnil(L);
		return 1;
	}

	if (index > 0) {
		index--;
		if (index >= luamysql->fields) {

			lua_pushnil(L);
			return 1;
		}
		else {
			pushmysqlfield(luamysql->row, luamysql->columns, L, index, lengths[index], luamysql->asstring);
			return 1;
		}
	}

	lua_createtable(L, 0, luamysql->fields);
	for (int n = 0; n < luamysql->fields; n++) {
		lua_pushlstring(L, luamysql->columns[n].name, luamysql->columns[n].name_length);
		pushmysqlfield(luamysql->row, luamysql->columns, L, n, lengths[n], luamysql->asstring);
		lua_settable(L, -3);
	}

	return 1;
}

LuaMySQLResult * lua_tomysqlresult(lua_State *L, int index) {

	LuaMySQLResult * luamysql = (LuaMySQLResult*)lua_touserdata(L, index);
	if (luamysql == NULL)
		luaL_error(L, "parameter is not a %s", LUAMYSQLRESULT);
	return luamysql;
}

LuaMySQLResult * luaL_checkmysqlresult(lua_State *L, int index) {

	LuaMySQLResult * luamysql = (LuaMySQLResult*)luaL_checkudata(L, index, LUAMYSQLRESULT);
	if (luamysql == NULL)
		luaL_error(L, "parameter is not a %s", LUAMYSQLRESULT);

	return luamysql;
}

LuaMySQLResult * lua_pushmysqlresult(lua_State *L) {

	LuaMySQLResult * luamysql = (LuaMySQLResult*)lua_newuserdata(L, sizeof(LuaMySQLResult));
	if (luamysql == NULL)
		luaL_error(L, "Unable to create mysqlresult");
	luaL_getmetatable(L, LUAMYSQLRESULT);
	lua_setmetatable(L, -2);
	memset(luamysql, 0, sizeof(LuaMySQLResult));

	return luamysql;
}

int luamysqlresult_gc(lua_State *L) {

	LuaMySQLResult * luamysql = (LuaMySQLResult*)lua_tomysqlresult(L, 1);

	if (luamysql->result) {
		mysql_free_result(luamysql->result);
		luamysql->result = NULL;
	}

	return 0;
}

int luamysqlresult_tostring(lua_State *L) {

	LuaMySQLResult * sq = lua_tomysqlresult(L, 1);
	char my[500];
	sprintf(my, "MySQLResult: 0x%08X", sq);
	lua_pushstring(L, my);
	return 1;
}