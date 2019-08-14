#include "odbc.h"

LuaOdbc* AssertIsOpen(lua_State* L, int idx) {

	LuaOdbc * odbc = lua_toodbc(L, 1);

	if (!odbc || !odbc->dbc) {
		lua_pop(L, lua_gettop(L));
		luaL_error(L, "Connection not open");
	}

	return odbc;
}

void PushDiagonstics(lua_State* L, SQLSMALLINT handleType, SQLHANDLE handle) {

	SQLCHAR state[6];
	SQLINTEGER* nativeError = NULL;
	SQLCHAR msg[254];
	SQLSMALLINT size;

	if (SQLGetDiagRec(handleType, handle, 1, state, nativeError, msg, sizeof(msg), &size) == SQL_SUCCESS) {
		
		msg[sizeof(msg)-1]='\0';
		
		if (size > sizeof(msg)) {
			size = sizeof(msg);
		}

		for (size_t i = size-1; i >= 0; i--)
		{
			if (msg[i] == '\n' || msg[i] == '\r' || msg[i] == ' ') {
				msg[i] = '\0';
			}
			else {
				break;
			}
		}	

		lua_pushfstring(L,"%s: %s", (const char*)state, (const char*)msg);
	}
	else {
		lua_pushstring(L, "No Message");
	}
}

int ODBCPrepare(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);
	size_t len;
	const char* sql = luaL_checklstring(L, 2, &len);

	if (odbc->stmt) {
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		odbc->stmt = NULL;
	}

	else if (SQLAllocHandle(SQL_HANDLE_STMT, odbc->dbc, &odbc->stmt) != SQL_SUCCESS) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}
	else if (SQLPrepare(odbc->stmt, (SQLCHAR*)sql, (SQLINTEGER)len)) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	odbc->paramnumber = 0;

	return 1;
}

int ODBCBind(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);

	if (!odbc->stmt) {
		luaL_error(L, "No statement is prepared, run Prepare first");
	}

	SQLSMALLINT valuetype;
	SQLSMALLINT paramtype;
	SQLSMALLINT paramlen;
	SQLSMALLINT decimal = 0;
	SQLUINTEGER len;
	SQLPOINTER data;

	switch (lua_type(L, 2)) {

		default:
			valuetype = SQL_C_CHAR;
			paramtype = SQL_CHAR;
			size_t rawlen;
			data = (SQLPOINTER)lua_tolstring(L, 2, &rawlen);
			len = rawlen;
			break;
	}

	if (SQLBindParameter(odbc->stmt, ++odbc->paramnumber, SQL_PARAM_INPUT, valuetype, paramtype, len, decimal, data, len+1, NULL) != SQL_SUCCESS) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int ODBCExecute(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);

	if (!odbc->stmt) {
		luaL_error(L, "No statement is prepared, run Prepare first");
	}

	if (!SUCCEEDED(SQLExecute(odbc->stmt))) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	return 1;
}

int ODBCFetch(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);

	if (!odbc->stmt) {
		luaL_error(L, "No statement is prepared, run Prepare first");
	}

	SQLRETURN ret = SQLFetch(odbc->stmt);

	if (!SUCCEEDED(ret)) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, ret == SQL_SUCCESS);

	return 1;
}

int ODBCGetResultColumns(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);

	if (!odbc->stmt) {
		luaL_error(L, "No statement is prepared, run Prepare first");
	}

	SQLSMALLINT cols;
	SQLCHAR columnname[254];
	SQLSMALLINT columnnamesize;
	SQLSMALLINT datatype;
	SQLUINTEGER columnsize;
	SQLSMALLINT decimaldigits;
	SQLSMALLINT nullable;
	SQLPOINTER data;
	SQLINTEGER datalen;

	if (!SUCCEEDED(SQLNumResultCols(odbc->stmt, &cols))) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_createtable(L, 0, cols);
	for (size_t i = 0; i < cols; i++)
	{
		if (!SUCCEEDED(SQLDescribeCol(odbc->stmt, i + 1, columnname, sizeof(columnname), &columnnamesize, &datatype, &columnsize, &decimaldigits, &nullable))) {
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);

			PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
			SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
			return 2;
		}

		lua_pushlstring(L, (const char*)columnname, columnnamesize);

		lua_pushinteger(L, datatype);

		lua_settable(L, -3);
	}

	return 1;
}

int ODBCGetRow(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);

	if (!odbc->stmt) {
		luaL_error(L, "No statement is prepared, run Prepare first");
	}

	SQLSMALLINT cols;

	if (!SUCCEEDED(SQLNumResultCols(odbc->stmt, &cols))) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}

	SQLCHAR columnname[254];
	SQLSMALLINT columnnamesize;
	SQLSMALLINT datatype;
	SQLUINTEGER columnsize;
	SQLSMALLINT decimaldigits;
	SQLSMALLINT nullable;
	SQLPOINTER data;
	SQLINTEGER datalen;
	SQLCHAR bit;

	lua_pop(L, lua_gettop(L));
	lua_createtable(L, 0, cols);
	for (size_t i = 0; i < cols; i++)
	{
		if (!SUCCEEDED(SQLDescribeCol(odbc->stmt, i+1, columnname, sizeof(columnname), &columnnamesize, &datatype, &columnsize, &decimaldigits, &nullable))) {
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);

			PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
			SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
			return 2;
		}
		
		lua_pushlstring(L, (const char*)columnname, columnnamesize);

		if (datatype == SQL_DECIMAL || datatype == SQL_NUMERIC || datatype == SQL_REAL || datatype == SQL_FLOAT || datatype == SQL_DOUBLE) {
			
			data = (SQLPOINTER)calloc(1, sizeof(lua_Number));

			if (!data) {
				luaL_error(L, "Unable to allocate memory");
			}

			if (!SUCCEEDED(SQLGetData(odbc->stmt, i+1, SQL_C_DOUBLE, data, sizeof(lua_Number), &datalen))) {

				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);

				PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
				SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
				return 2;
			}

			if (datalen < 0) {
				lua_pushnil(L);
			}
			else {
				lua_pushnumber(L, (lua_Number) * (lua_Number*)data);
			}
		}
		else if (datatype == SQL_SMALLINT || datatype == SQL_INTEGER ||datatype == SQL_TINYINT || datatype == SQL_BIGINT) {

			data = (SQLPOINTER)calloc(1, sizeof(lua_Integer));

			if (!data) {
				luaL_error(L, "Unable to allocate memory");
			}

			if (!SUCCEEDED(SQLGetData(odbc->stmt, i + 1, SQL_C_SLONG, data, sizeof(lua_Integer), &datalen))) {

				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);

				PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
				SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
				return 2;
			}

			if (datalen < 0) {
				lua_pushnil(L);
			}
			else {
				lua_pushinteger(L, (lua_Integer) * (lua_Integer*)data);
			}
		}
		else if (datatype == SQL_BIT) {

			if (!SUCCEEDED(SQLGetData(odbc->stmt, i + 1, SQL_C_BIT, &bit, sizeof(SQLCHAR), &datalen))) {

				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);

				PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
				SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
				return 2;
			}

			if (datalen < 0) {
				lua_pushnil(L);
			}
			else {
				lua_pushboolean(L, bit > 0);
			}
		}
		else {

			data = calloc(sizeof(SQLCHAR), columnsize);

			if (!data) {
				luaL_error(L, "Unable to allocate memory");
			}

			if (!SUCCEEDED(SQLGetData(odbc->stmt, i + 1, SQL_C_CHAR, data, columnsize, &datalen))) {

				lua_pop(L, lua_gettop(L));
				lua_pushboolean(L, false);

				PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
				SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
				return 2;
			}

			if (datalen < 0) {
				lua_pushnil(L);
			}
			else {
				lua_pushlstring(L, (const char*)data, datalen);
			}
		}

		if (data) {
			free(data);
			data = NULL;
		}

		lua_settable(L, -3);
	}

	return 1;
}

int ODBCGetAllDrivers(lua_State* L) {

	SQLHENV env;
	SQLCHAR driver[256];
	SQLCHAR attr[256];
	SQLSMALLINT driver_ret;
	SQLSMALLINT attr_ret;
	SQLRETURN ret;

	if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env) != SQL_SUCCESS) {

		lua_pushnil(L);
		PushDiagonstics(L, SQL_HANDLE_ENV, env);
		SQLFreeHandle(SQL_HANDLE_ENV, env);
		return 2;
	}
	else if (SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0) != SQL_SUCCESS) {

		lua_pushnil(L);
		PushDiagonstics(L, SQL_HANDLE_ENV, env);
		SQLFreeHandle(SQL_HANDLE_ENV, env);
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_createtable(L, 0, 0);

	int n = 0;
	ret = SQLDrivers(env, SQL_FETCH_FIRST, driver, sizeof(driver), &driver_ret, attr, sizeof(attr), &attr_ret);
	while (SUCCEEDED(ret) && ret != SQL_NO_DATA) {

		lua_pushlstring(L, (const char*)driver, driver_ret);

		lua_rawseti(L, -2, ++n);
		ret = SQLDrivers(env, SQL_FETCH_NEXT, driver, sizeof(driver), &driver_ret, attr, sizeof(attr), &attr_ret);
	}

	SQLFreeHandle(SQL_HANDLE_ENV, env);

	return 1;
}

int ODBCDriverConnect(lua_State* L) {

	size_t len;
	const char* connectionString = luaL_checklstring(L, 1, &len);

	if (!connectionString || len <= 0) {
		luaL_error(L, "Connection string cannot be empty");
	}

	char* currentconnectionstring = (char*)calloc(len + 1, sizeof(char));
	if (!currentconnectionstring) {
		luaL_error(L, "Failed to allocate memory");
	}
	memcpy(currentconnectionstring, connectionString, len);

	lua_pop(L, lua_gettop(L));

	LuaOdbc* odbc = lua_pushodbc(L);

	if (!odbc) {
		free(currentconnectionstring);
		luaL_error(L, "Failed to allocate memory");
	}

	odbc->ConnectionString = currentconnectionstring;

	if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &odbc->env) != SQL_SUCCESS) {

		lua_pushnil(L);
		PushDiagonstics(L, SQL_HANDLE_ENV, odbc->env);
		return 2;
	}
	else if (SQLSetEnvAttr(odbc->env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0) != SQL_SUCCESS) {

		lua_pushnil(L);
		PushDiagonstics(L, SQL_HANDLE_ENV, odbc->env);
		return 2;
	}
	else if (SQLAllocHandle(SQL_HANDLE_DBC, odbc->env, &odbc->dbc) != SQL_SUCCESS) {

		lua_pushnil(L);
		PushDiagonstics(L, SQL_HANDLE_DBC, odbc->env);
		return 2;
	}
	else if (SQLDriverConnect(odbc->dbc, NULL, (SQLCHAR*)odbc->ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE) != SQL_SUCCESS) {

		lua_pushnil(L);
		PushDiagonstics(L, SQL_HANDLE_DBC, odbc->dbc);
		return 2;
	}

	return 1;
}


LuaOdbc* lua_pushodbc(lua_State* L) {
	LuaOdbc* odbc = (LuaOdbc*)lua_newuserdata(L, sizeof(LuaOdbc));
	if (odbc == NULL)
		return NULL;
	luaL_getmetatable(L, ODBC);
	lua_setmetatable(L, -2);
	memset(odbc, 0, sizeof(LuaOdbc));

	return odbc;
}

LuaOdbc* lua_toodbc(lua_State* L, int index) {
	LuaOdbc* odbc = (LuaOdbc*)luaL_checkudata(L, index, ODBC);
	if (odbc == NULL)
		luaL_error(L, "parameter is not a %s", ODBC);
	return odbc;
}

int odbc_gc(lua_State* L) {

	LuaOdbc* odbc = lua_toodbc(L, 1);

	if (odbc->ConnectionString) {
		free(odbc->ConnectionString);
		odbc->ConnectionString = NULL;
	}

	if (odbc->stmt) {
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		odbc->stmt = NULL;
	}

	if (odbc->dbc) {
		SQLDisconnect(odbc->dbc);
		SQLFreeHandle(SQL_HANDLE_DBC, odbc->dbc);
		odbc->dbc = NULL;
	}
	
	if (odbc->env) {
		SQLFreeHandle(SQL_HANDLE_ENV, odbc->env);
		odbc->env = NULL;
	}

	return 0;
}

int odbc_tostring(lua_State* L) {
	char tim[1024];
	sprintf(tim, "ODBC: 0x%08X", lua_toodbc(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}