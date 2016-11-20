#include "LuaMySQL.h"
#include <string.h>
#define CR_SERVER_GONE_ERROR 2006
#pragma comment(lib, "mysql/libmysql.lib")

int DataToHex(lua_State *L){
	
	size_t len;
	const char * data = luaL_checklstring(L, 1, &len);

	char * buffer = (char*)malloc((len*2)+3);
	if (!buffer){
		luaL_error(L, "Unable to allocate %u bytes for conversion buffer", (len * 2) + 1);
	}

	buffer[0] = '0';
	buffer[1] = 'x';

	unsigned long newlen = mysql_hex_string(&buffer[2], data, len);
	lua_pop(L, 1);
	lua_pushlstring(L, buffer, newlen+2);
	free(buffer);
	return 1;
}

void pushmysqlfield(LuaMySQL * luamysql, lua_State *L, int n, unsigned long length){

	LUA_NUMBER number;
	LUA_INTEGER integer;

	if (luamysql->row[n] == NULL){
		lua_pushnil(L);
	}
	else{
		switch (luamysql->columns[n].type){

		case MYSQL_TYPE_BIT:
			lua_pushboolean(L, luamysql->row[n][0] != '0');
			break;
		case MYSQL_TYPE_TINY:
		case MYSQL_TYPE_SHORT:
		case MYSQL_TYPE_LONG:
		case MYSQL_TYPE_LONGLONG:
		case MYSQL_TYPE_INT24:
			sscanf_s(luamysql->row[n], "%lld", &integer);
			lua_pushinteger(L, integer);
			break;
		case MYSQL_TYPE_FLOAT:
		case MYSQL_TYPE_DOUBLE:
			sscanf_s(luamysql->row[n], "%lf", &number);
			lua_pushinteger(L, number);
			break;
		default:
			lua_pushlstring(L, luamysql->row[n], length);
			break;
		}
	}
}

int MySQLGetRow(lua_State *L){

	LuaMySQL * luamysql = luaL_checkmysql(L, 1);
	unsigned long *lengths;
	int index = luaL_optinteger(L, 2, -1);

	lua_pop(L, lua_gettop(L));

	if (luamysql->row == NULL || luamysql->result == NULL || luamysql->columns == NULL){
		lua_pushnil(L);
		return 1;
	}

	lengths = mysql_fetch_lengths(luamysql->result);

	if (index > 0){
		index--;
		if (index >= luamysql->fields){

			lua_pushnil(L);
			return 1;
		}
		else{
			pushmysqlfield(luamysql, L, index, lengths[index]);
			return 1;
		}
	}
	
	lua_createtable(L, 0, luamysql->fields);
	for (int n = 0; n < luamysql->fields; n++){
		lua_pushlstring(L, luamysql->columns[n].name, luamysql->columns[n].name_length);
		pushmysqlfield(luamysql, L, n, lengths[n]);
		lua_settable(L, -3);
	}

	return 1;
}

int MySQLFetch(lua_State *L){

	LuaMySQL * luamysql = luaL_checkmysql(L, 1);

	if (luamysql->result == NULL){
		lua_pop(L, 1);
		lua_pushboolean(L, false);
		return 1;
	}

	luamysql->row = mysql_fetch_row(luamysql->result);
	if (luamysql->row == NULL){

		mysql_free_result(luamysql->result);
		luamysql->result = NULL;
		luamysql->row = NULL;
		luamysql->columns = NULL;

		lua_pop(L, 1);
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pop(L, 1);
	lua_pushboolean(L, true);
	return 1;
}

int MySQLExecute(lua_State *L){

	size_t len;
	unsigned long strlen;
	LuaMySQL * luamysql = luaL_checkmysql(L, 1);
	const char * query = luaL_checklstring(L, 2, &len);

	if (luamysql->result){
		mysql_free_result(luamysql->result);
		luamysql->result = NULL;
		luamysql->row = NULL;
		luamysql->columns = NULL;
	}

	if (luamysql->server == NULL || luamysql->server[0] == '\0'){
		luaL_error(L, "No connection initilized, call Connect");
	}

	if (!luamysql->connection)
	{
		if (!Reconnect(luamysql))
		{
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushstring(L, mysql_error(&luamysql->mysql));
			return 2;
		}
	}

	if (mysql_query(luamysql->connection, query) != 0){

		unsigned int error_no = mysql_errno(&luamysql->mysql);
		if (error_no == CR_SERVER_GONE_ERROR && Reconnect(luamysql)){
			if (mysql_query(luamysql->connection, (const char *)query) != 0){
				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);
				lua_pushstring(L, mysql_error(&luamysql->mysql));
				return 2;
			}
		}
		else{
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushstring(L, mysql_error(&luamysql->mysql));
			return 2;
		}
	}

	luamysql->result = mysql_store_result(&luamysql->mysql);
	if (luamysql->result){
		luamysql->fields = mysql_num_fields(luamysql->result);
		luamysql->columns = mysql_fetch_fields(luamysql->result);
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, true);
		lua_pushinteger(L, mysql_num_rows(luamysql->result));
	}
	else{
		if (mysql_field_count(&luamysql->mysql) == 0)
		{
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, true);
			lua_pushinteger(L, mysql_affected_rows(&luamysql->mysql));
		}
		else
		{
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);
			lua_pushstring(L, mysql_error(&luamysql->mysql));
		}
	}

	return 2;
}

int MySQLConnect(lua_State *L){

	LuaMySQL * luamysql = lua_pushmysql(L);
	if (!mysql_init(&luamysql->mysql)){
		luaL_error(L, "Unable to initialize mysql");
	}
	size_t len;
	const char * server = luaL_checklstring(L, 1, &len);
	luamysql->server = (char*)malloc(len + 1);
	luamysql->server[len] = '\0';
	memcpy(luamysql->server, server, len);

	const char * user = luaL_checklstring(L, 2, &len);
	luamysql->user = (char*)malloc(len + 1);
	luamysql->user[len] = '\0';
	memcpy(luamysql->user, server, len);

	const char * password = luaL_checklstring(L, 3, &len);
	luamysql->password = (char*)malloc(len + 1);
	luamysql->password[len] = '\0';
	memcpy(luamysql->password, server, len);

	const char * schema = luaL_checklstring(L, 4, &len);
	luamysql->schema = (char*)malloc(len + 1);
	luamysql->schema[len] = '\0';
	memcpy(luamysql->schema, server, len);

	unsigned long port = luaL_checkinteger(L, 5);
	luamysql->port = port;

	luamysql->connection = mysql_real_connect(&luamysql->mysql, server, user, password, schema, port, NULL, NULL);
	if (luamysql->connection == NULL)
	{
		mysql_close(&luamysql->mysql);
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}

	return 1;
}

bool Reconnect(LuaMySQL *luamysql){

	mysql_close(&luamysql->mysql);
	luamysql->connection = mysql_real_connect(&luamysql->mysql, luamysql->server, luamysql->user, luamysql->password, luamysql->schema, luamysql->port, NULL, NULL);
	if (luamysql->connection == NULL)
	{
		mysql_close(&luamysql->mysql);
		return FALSE;
	}

	return TRUE;
}

LuaMySQL * lua_tomysql(lua_State *L, int index){

	LuaMySQL * luamysql = (LuaMySQL*)lua_touserdata(L, index);
	if (luamysql == NULL)
		luaL_error(L, "paramter is not a %s", LUAMYSQL);
	return luamysql;
}

LuaMySQL * luaL_checkmysql(lua_State *L, int index){

	LuaMySQL * luamysql = (LuaMySQL*)luaL_checkudata(L, index, LUAMYSQL);
	if (luamysql == NULL)
		luaL_error(L, "paramter is not a %s", LUAMYSQL);
	return luamysql;
}

LuaMySQL * lua_pushmysql(lua_State *L){

	LuaMySQL * luamysql = (LuaMySQL*)lua_newuserdata(L, sizeof(LuaMySQL));
	if (luamysql == NULL)
		luaL_error(L, "Unable to create mysql connection");
	luaL_getmetatable(L, LUAMYSQL);
	lua_setmetatable(L, -2);
	memset(luamysql, 0, sizeof(LuaMySQL));
	return luamysql;
}

int luamysql_gc(lua_State *L){

	LuaMySQL * luamysql = (LuaMySQL*)lua_tomysql(L, 1);

	if (luamysql->result){
		mysql_free_result(luamysql->result);
		luamysql->result = NULL;
	}

	if (luamysql->connection){
		mysql_close(&luamysql->mysql);
		luamysql->connection = NULL;
	}

	if (luamysql->server){
		free(luamysql->server);
		luamysql->server = NULL;
	}

	if (luamysql->user){
		free(luamysql->user);
		luamysql->user = NULL;
	}

	if (luamysql->password){
		free(luamysql->password);
		luamysql->password = NULL;
	}

	if (luamysql->schema){
		free(luamysql->schema);
		luamysql->schema = NULL;
	}

	return 0;
}

int luamysql_tostring(lua_State *L){

	lua_pushfstring(L, "Timer: 0x%08X", lua_tomysql(L, 1));
	return 1;
}