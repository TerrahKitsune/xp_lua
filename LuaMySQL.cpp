#include "LuaMySQL.h"
#include <string.h>
#define CR_SERVER_GONE_ERROR 2006
#pragma comment(lib, "mysql/libmysql.lib")
#include <ppltasks.h>

using namespace concurrency;

void CleanUp(LuaAsyncResult * result) {

	if (!result)
		return;

	if (result->Error) {
		free(result->Error);
		result->Error = NULL;
	}

	free(result);
}


int DataToHex(lua_State *L) {

	size_t len;
	const char * data = luaL_checklstring(L, 1, &len);

	if (data == NULL) {
		luaL_error(L, "No data given to encode");
	}

	char * buffer = (char*)malloc((len * 2) + 3);
	if (!buffer) {
		luaL_error(L, "Unable to allocate %u bytes for conversion buffer", (len * 2) + 1);
	}

	buffer[0] = '0';
	buffer[1] = 'x';

	unsigned long newlen = mysql_hex_string(&buffer[2], data, len);

	lua_pop(L, 1);
	if (newlen <= 0)
		lua_pushstring(L, "0x0");
	else
		lua_pushlstring(L, buffer, newlen + 2);
	free(buffer);
	return 1;
}

int EscapeString(lua_State *L) {

	LuaMySQL * luamysql = luaL_checkmysql(L, 1);

	size_t len;
	const char * data = luaL_checklstring(L, 2, &len);

	if (data == NULL) {
		luaL_error(L, "No data given to escape");
	}

	if (!luamysql->connection && !Reconnect(luamysql)) {
		luaL_error(L, "Mysql is not connected");
	}

	char * buffer = (char*)malloc((len * 2) + 1);
	if (!buffer) {
		luaL_error(L, "Unable to allocate %u bytes for conversion buffer", (len * 2) + 1);
	}

	unsigned long newlen = mysql_real_escape_string(luamysql->connection, buffer, data, len);
	lua_pop(L, 1);
	lua_pushlstring(L, buffer, newlen);
	free(buffer);
	return 1;
}

int MySQLSetAsString(lua_State *L) {
	LuaMySQL * luamysql = luaL_checkmysql(L, 1);
	luamysql->asstring = lua_toboolean(L, 2) > 0;
	lua_pop(L, lua_gettop(L));
	return 0;
}


int MySQLGetRow(lua_State *L) {

	LuaMySQL * luamysql = luaL_checkmysql(L, 1);

	if (luamysql->isRunningAsync) {
		luaL_error(L, "Async query is running, wait for it to finish before doing operations with this mysql connection");
	}

	unsigned long *lengths;
	int index = (int)luaL_optinteger(L, 2, -1);

	lua_pop(L, lua_gettop(L));

	if (luamysql->row == NULL || luamysql->result == NULL || luamysql->columns == NULL || luamysql->connection == NULL) {
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

LuaAsyncResult * GetResults(LuaMySQL * luamysql) {

	if (!luamysql->hasTask) {
		return NULL;
	}
	else {
		luamysql->hasTask = false;
	}

	luamysql->task.wait();
	return luamysql->task.get();
}

int MySQLFetch(lua_State *L) {

	LuaMySQL * luamysql = luaL_checkmysql(L, 1);

	LuaAsyncResult* results = GetResults(luamysql);

	if (results && !results->Ok) {
		lua_pop(L, lua_gettop(L));
		lua_pushstring(L, results->Error);
		CleanUp(results);
		lua_error(L);
	}
	else {
		CleanUp(results);
	}

	if (luamysql->result == NULL || luamysql->connection == NULL) {
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

int MySQLForkResult(lua_State *L) {

	LuaMySQL * luamysql = luaL_checkmysql(L, 1);

	LuaAsyncResult* results = GetResults(luamysql);

	lua_pop(L, lua_gettop(L));

	if (results && !results->Ok) {
		lua_pushstring(L, results->Error);
		CleanUp(results);
		lua_error(L);
	}
	else {
		CleanUp(results);
	}

	LuaMySQLResult * result = lua_pushmysqlresult(L);

	result->result = luamysql->result;
	result->columns = luamysql->columns;
	result->fields = luamysql->fields;
	result->row = luamysql->row;
	result->rows = (int)(luamysql->result ? mysql_num_rows(luamysql->result) : 0);
	result->asstring = luamysql->asstring;

	luamysql->result = NULL;
	luamysql->columns = NULL;
	result->fields = NULL;
	result->row = NULL;

	return 1;
}

void SetResult(LuaAsyncResult * result, const char *error, int rows) {

	if (result->Error) {
		free(result->Error);
		result->Error = NULL;
	}

	if (error) {
		result->Error = (char*)calloc(strlen(error) + 1, 1);

		if (result->Error) {
			strcpy(result->Error, error);
		}

		result->Ok = false;
	}
	else {
		result->Ok = true;
		result->Rows = rows;
	}
}

LuaAsyncResult* Execute(LuaMySQL * luamysql, bool store) {

	LuaAsyncResult * result = (LuaAsyncResult*)calloc(1, sizeof(LuaAsyncResult));

	if (!luamysql->connection)
	{
		if (!Reconnect(luamysql))
		{
			SetResult(result, luamysql->lastError, 0);
			return result;
		}
	}

	if (mysql_query(luamysql->connection, luamysql->query) != 0) {

		unsigned int error_no = mysql_errno(luamysql->mysql);
		if (error_no == CR_SERVER_GONE_ERROR && Reconnect(luamysql)) {
			if (mysql_query(luamysql->connection, (const char *)luamysql->query) != 0) {

				SetResult(result, mysql_error(luamysql->mysql), 0);
				return result;
			}
		}
		else {

			const char * err = mysql_error(luamysql->mysql);

			if (err && luamysql->mysql) {
				SetResult(result, err, 0);
			}
			else {
				SetResult(result, luamysql->lastError, 0);
			}
			return result;
		}
	}
	
	luamysql->result = store ? mysql_store_result(luamysql->connection) : mysql_use_result(luamysql->connection);
	if (luamysql->result) {
		SetResult(result, NULL, (int)mysql_num_rows(luamysql->result));
		return result;
	}
	else {
		if (mysql_field_count(luamysql->mysql) == 0)
		{
			SetResult(result, NULL, (int)mysql_affected_rows(luamysql->connection));
			return result;
		}
		else
		{
			SetResult(result, mysql_error(luamysql->mysql), 0);
		}
	}

	return result;
}

int MySQLIsRunningAsync(lua_State *L) {
	LuaMySQL * luamysql = luaL_checkmysql(L, 1);
	lua_pop(L, lua_gettop(L));
	if (!luamysql->hasTask) {
		lua_pushboolean(L, false);
	}
	else {
		lua_pushboolean(L, luamysql->isRunningAsync || !luamysql->task.is_done());
	}
	return 1;
}

int MySQLGetAsyncResults(lua_State *L) {

	LuaMySQL * luamysql = luaL_checkmysql(L, 1);

	if (!luamysql->hasTask) {
		lua_pushboolean(L, true);
		lua_pushstring(L, "No result");
		return 2;
	}

	luamysql->task.wait();

	LuaAsyncResult * result = luamysql->task.get();

	if (result) {

		lua_pushboolean(L, result->Ok);
		if (!result->Ok) {
			lua_pushstring(L, result->Error);
		}
		else {
			lua_pushinteger(L, result->Rows);
		}
	}
	else {
		lua_pushboolean(L, true);
		lua_pushstring(L, "No result");
	}

	return 2;
}

int MySQLExecute(lua_State *L) {

	size_t len;
	LuaMySQL * luamysql = luaL_checkmysql(L, 1);

	if (luamysql->hasTask) {
		luamysql->task.wait();
	}

	const char * query = luaL_checklstring(L, 2, &len);

	int runasync = 0;

	if (lua_gettop(L) >= 3) {
		runasync = lua_toboolean(L, 3);
	}

	if (luamysql->hasTask) {
		try {
			luamysql->task.wait();
		}
		catch (...) {}
		CleanUp(luamysql->task.get());
		luamysql->hasTask = false;
	}

	if (luamysql->result) {

		mysql_free_result(luamysql->result);

		luamysql->result = NULL;
		luamysql->row = NULL;
		luamysql->columns = NULL;
		luamysql->fields = NULL;
	}

	if (luamysql->server == NULL || luamysql->server[0] == '\0') {
		luaL_error(L, "No connection initilized, call Connect");
	}

	if (luamysql->query) {
		free(luamysql->query);
	}

	luamysql->query = (char*)calloc(len + 1, sizeof(char));

	if (!luamysql->query) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Unable to allocate memory");
		return 2;
	}
	memcpy(luamysql->query, query, len);
	lua_pop(L, lua_gettop(L));
	if (!runasync) {

		LuaAsyncResult * result = Execute(luamysql, false);

		if (result) {

			lua_pushboolean(L, result->Ok);
			if (!result->Ok) {
				lua_pushstring(L, result->Error);
			}
			else {
				lua_pushinteger(L, result->Rows);
			}
			CleanUp(result);
		}
		else {
			lua_pushboolean(L, false);
			lua_pushstring(L, "Unable to allocate memory");
		}
	}
	else {

		luamysql->isRunningAsync = true;
		luamysql->hasTask = true;
		luamysql->task = create_task([luamysql]
		{
			try {
				LuaAsyncResult * result = Execute(luamysql, true);
				luamysql->isRunningAsync = false;
				//if (luamysql->result)
				//	mysql_free_result(luamysql->result);
				return result;
			}
			catch (...) {
				//if (luamysql->result)
				//	mysql_free_result(luamysql->result);
				luamysql->isRunningAsync = false;
			}

			return (LuaAsyncResult*)NULL;
		});

		lua_pushboolean(L, true);
		lua_pushstring(L, "Running async");
	}

	return 2;
}

int MySQLChangeDatabase(lua_State *L) {

	LuaMySQL * luamysql = luaL_checkmysql(L, 1);
	const char * db = luaL_checkstring(L, 2);

	if (luamysql->hasTask) {
		luamysql->task.wait();
	}

	if(luamysql->schema)
		free(luamysql->schema);
	luamysql->schema = (char*)calloc(strlen(db) + 1, sizeof(char));

	if (!luamysql->schema) {
		lua_pop(L, lua_gettop(L));
		luaL_error(L, "Unable to alloc memory");
		return 0;
	}

	strcpy(luamysql->schema, db);
	lua_pop(L, 1);

	lua_pushfstring(L, "USE `%s`;", luamysql->schema);

	return MySQLExecute(L);
}

int MySQLConnect(lua_State *L) {

	for (int n = 1; n <= 4; n++) {
		if (!lua_isstring(L, n))
			luaL_error(L, "Parameter #%d on Connect is not a string", n);
	}

	size_t len;
	const char * temp = luaL_checklstring(L, 1, &len);
	char * temp_server = (char*)malloc(len + 1);
	temp_server[len] = '\0';
	memcpy(temp_server, temp, len);

	temp = luaL_checklstring(L, 2, &len);
	char * temp_user = (char*)malloc(len + 1);
	temp_user[len] = '\0';
	memcpy(temp_user, temp, len);

	temp = luaL_checklstring(L, 3, &len);
	char * temp_password = (char*)malloc(len + 1);
	temp_password[len] = '\0';
	memcpy(temp_password, temp, len);

	temp = luaL_checklstring(L, 4, &len);
	char * temp_schema = (char*)malloc(len + 1);
	temp_schema[len] = '\0';
	memcpy(temp_schema, temp, len);

	int port = (int)luaL_optinteger(L, 5, 3306);
	int timeout = (int)luaL_optinteger(L, 6, -1);

	lua_pop(L, lua_gettop(L));

	LuaMySQL * luamysql = lua_pushmysql(L);

	luamysql->mysql = (MYSQL*)calloc(1, sizeof(MYSQL));

	if (!luamysql->mysql) {
		free(temp_server);
		free(temp_user);
		free(temp_password);
		free(temp_schema);
		luaL_error(L, "Unable to allocate memory for mysql");
	}

	if (!mysql_init(luamysql->mysql)) {
		free(temp_server);
		free(temp_user);
		free(temp_password);
		free(temp_schema);
		luaL_error(L, "Unable to initialize mysql");
	}

	

	if (timeout <= 0)
		timeout = 3600;

	luamysql->timeout = timeout;

	//Give the memory to lua
	luamysql->server = temp_server;
	luamysql->user = temp_user;
	luamysql->password = temp_password;
	luamysql->schema = temp_schema;
	luamysql->port = port;

	mysql_options(luamysql->mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	mysql_options(luamysql->mysql, MYSQL_OPT_READ_TIMEOUT, &timeout);
	mysql_options(luamysql->mysql, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

	luamysql->connection = mysql_real_connect(luamysql->mysql, temp_server, temp_user, temp_password, temp_schema, port, NULL, NULL);
	if (luamysql->connection == NULL)
	{
		mysql_close(luamysql->mysql);
		free(luamysql->mysql);
		luamysql->mysql = NULL;

		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}
	else {
		mysql_options(luamysql->connection, MYSQL_OPT_CONNECT_TIMEOUT, &luamysql->timeout);
		mysql_options(luamysql->connection, MYSQL_OPT_READ_TIMEOUT, &luamysql->timeout);
		mysql_options(luamysql->connection, MYSQL_OPT_WRITE_TIMEOUT, &luamysql->timeout);
	}

	return 1;
}

int SetTimeout(lua_State *L) {

	LuaMySQL * luamysql = luaL_checkmysql(L, 1);

	if (luamysql->isRunningAsync) {
		luaL_error(L, "Async query is running, wait for it to finish before doing operations with this mysql connection");
	}

	if (luamysql) {

		luamysql->timeout = (int)min(luaL_checkinteger(L, 2), 1);

		mysql_options(luamysql->mysql, MYSQL_OPT_CONNECT_TIMEOUT, &luamysql->timeout);
		mysql_options(luamysql->mysql, MYSQL_OPT_READ_TIMEOUT, &luamysql->timeout);
		mysql_options(luamysql->mysql, MYSQL_OPT_WRITE_TIMEOUT, &luamysql->timeout);

		if (luamysql->connection)
		{
			mysql_options(luamysql->connection, MYSQL_OPT_CONNECT_TIMEOUT, &luamysql->timeout);
			mysql_options(luamysql->connection, MYSQL_OPT_READ_TIMEOUT, &luamysql->timeout);
			mysql_options(luamysql->connection, MYSQL_OPT_WRITE_TIMEOUT, &luamysql->timeout);
		}
	}

	return 0;
}

bool Reconnect(LuaMySQL *luamysql) {

	if (luamysql->result) {

		mysql_free_result(luamysql->result);

		luamysql->result = NULL;
		luamysql->row = NULL;
		luamysql->columns = NULL;
		luamysql->fields = NULL;
	}

	if (luamysql->connection)
		mysql_close(luamysql->connection);

	if (luamysql->mysql) {
		mysql_close(luamysql->mysql);
		free(luamysql->mysql);
		luamysql->mysql = NULL;
	}

	luamysql->mysql = (MYSQL*)calloc(1, sizeof(MYSQL));

	if (!luamysql->mysql || !mysql_init(luamysql->mysql)) {
		return FALSE;
	}

	mysql_options(luamysql->mysql, MYSQL_OPT_CONNECT_TIMEOUT, &luamysql->timeout);
	mysql_options(luamysql->mysql, MYSQL_OPT_READ_TIMEOUT, &luamysql->timeout);
	mysql_options(luamysql->mysql, MYSQL_OPT_WRITE_TIMEOUT, &luamysql->timeout);

	luamysql->connection = mysql_real_connect(luamysql->mysql, luamysql->server, luamysql->user, luamysql->password, luamysql->schema, luamysql->port, NULL, NULL);
	if (luamysql->connection == NULL)
	{
		const char * err = mysql_error(luamysql->mysql);

		if (!err) {
			err = "Unknown error reconnecting";
		}

		if (luamysql->lastError) {
			free(luamysql->lastError);
		}

		luamysql->lastError = (char*)malloc(strlen(err)+1);

		if (luamysql->lastError) {
			strcpy(luamysql->lastError, err);
		}

		mysql_close(luamysql->mysql);
		free(luamysql->mysql);
		luamysql->mysql = NULL;
		return FALSE;
	}

	return TRUE;
}

LuaMySQL * lua_tomysql(lua_State *L, int index) {

	LuaMySQL * luamysql = (LuaMySQL*)lua_touserdata(L, index);
	if (luamysql == NULL)
		luaL_error(L, "parameter is not a %s", LUAMYSQL);
	return luamysql;
}

LuaMySQL * luaL_checkmysql(lua_State *L, int index) {

	LuaMySQL * luamysql = (LuaMySQL*)luaL_checkudata(L, index, LUAMYSQL);
	if (luamysql == NULL)
		luaL_error(L, "parameter is not a %s", LUAMYSQL);

	if (luamysql->user == NULL ||
		luamysql->password == NULL ||
		luamysql->schema == NULL ||
		luamysql->server == NULL) {
		luaL_error(L, "Mysql connection uninitilized!");
	}

	return luamysql;
}

LuaMySQL * lua_pushmysql(lua_State *L) {

	LuaMySQL * luamysql = (LuaMySQL*)lua_newuserdata(L, sizeof(LuaMySQL));
	if (luamysql == NULL)
		luaL_error(L, "Unable to create mysql connection");
	luaL_getmetatable(L, LUAMYSQL);
	lua_setmetatable(L, -2);
	memset(luamysql, 0, sizeof(LuaMySQL));

	return luamysql;
}

int luamysql_gc(lua_State *L) {

	LuaMySQL * luamysql = (LuaMySQL*)lua_tomysql(L, 1);

	if (luamysql->hasTask) {
		try {
			luamysql->task.wait();
		}
		catch (...) {}
		CleanUp(luamysql->task.get());
	}

	if (luamysql->result) {
		mysql_free_result(luamysql->result);
		luamysql->result = NULL;
	}

	if (luamysql->connection) {
		mysql_close(luamysql->mysql);
		free(luamysql->mysql);
		luamysql->connection = NULL;
	}

	if (luamysql->server) {
		free(luamysql->server);
		luamysql->server = NULL;
	}

	if (luamysql->user) {
		free(luamysql->user);
		luamysql->user = NULL;
	}

	if (luamysql->password) {
		free(luamysql->password);
		luamysql->password = NULL;
	}

	if (luamysql->schema) {
		free(luamysql->schema);
		luamysql->schema = NULL;
	}

	if (luamysql->query) {
		free(luamysql->query);
		luamysql->query = NULL;
	}

	if (luamysql->lastError) {
		free(luamysql->lastError);
	}

	return 0;
}

int luamysql_tostring(lua_State *L) {

	LuaMySQL * sq = lua_tomysql(L, 1);
	char my[1024];
	sprintf(my, "MySQL: 0x%08X Connection: server=%s;database=%s;uid=%s;port=%u;", sq, sq->server, sq->schema, sq->user, sq->port);
	lua_pushstring(L, my);
	return 1;
}