#include "odbc.h"
#include <math.h>
#include "luawchar.h"

LuaOdbc* AssertIsOpen(lua_State* L, int idx) {

	LuaOdbc* odbc = lua_toodbc(L, 1);

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

		msg[sizeof(msg) - 1] = '\0';

		if (size > sizeof(msg)) {
			size = sizeof(msg);
		}

		for (size_t i = size - 1; i >= 0; i--)
		{
			if (msg[i] == '\n' || msg[i] == '\r' || msg[i] == ' ') {
				msg[i] = '\0';
			}
			else {
				break;
			}
		}

		lua_pushfstring(L, "%s: %s", (const char*)state, (const char*)msg);
	}
	else {
		lua_pushstring(L, "No Message");
	}
}

int ODBCToggleAutoCommit(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);
	bool autocommit = lua_toboolean(L, 2) > 0;
	SQLPOINTER val = autocommit ? (SQLPOINTER)SQL_AUTOCOMMIT_ON : (SQLPOINTER)SQL_AUTOCOMMIT_OFF;


	if (!SQL_SUCCEEDED(SQLSetConnectAttr(odbc->dbc, SQL_ATTR_AUTOCOMMIT, val, SQL_IS_UINTEGER))) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_DBC, odbc->dbc);
		SQLFreeHandle(SQL_HANDLE_DBC, odbc->dbc);
		return 2;
	}

	lua_pushboolean(L, true);
	return 1;
}

int ODBCBegin(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);

	if (!SQL_SUCCEEDED(SQLSetConnectAttr(odbc->dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER))) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_DBC, odbc->dbc);
		SQLFreeHandle(SQL_HANDLE_DBC, odbc->dbc);
		return 2;
	}

	lua_pushboolean(L, true);
	return 1;
}

int ODBCCommit(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);

	if (!SQL_SUCCEEDED(SQLEndTran(SQL_HANDLE_DBC, odbc->dbc, SQL_COMMIT))) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_DBC, odbc->dbc);
		SQLFreeHandle(SQL_HANDLE_DBC, odbc->dbc);
		return 2;
	}
	else if (!SQL_SUCCEEDED(SQLSetConnectAttr(odbc->dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER))) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_DBC, odbc->dbc);
		SQLFreeHandle(SQL_HANDLE_DBC, odbc->dbc);
		return 2;
	}

	lua_pushboolean(L, true);
	return 1;
}

int ODBCRollback(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);

	if (!SQL_SUCCEEDED(SQLEndTran(SQL_HANDLE_DBC, odbc->dbc, SQL_ROLLBACK))) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_DBC, odbc->dbc);
		SQLFreeHandle(SQL_HANDLE_DBC, odbc->dbc);
		return 2;
	}
	else if (!SQL_SUCCEEDED(SQLSetConnectAttr(odbc->dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER))) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_DBC, odbc->dbc);
		SQLFreeHandle(SQL_HANDLE_DBC, odbc->dbc);
		return 2;
	}

	lua_pushboolean(L, true);
	return 1;
}

void ClearStatement(LuaOdbc* odbc) {

	if (odbc->params) {

		for (unsigned int i = 0; i < odbc->numbparams; i++)
		{
			if (odbc->params[i])
				gff_free(odbc->params[i]);
		}

		gff_free(odbc->params);

		odbc->params = NULL;
		odbc->numbparams = 0;
	}

	if (odbc->stmt) {
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		odbc->stmt = NULL;
	}
}

int ODBCPrepare(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);
	size_t len;
	const char* sql = luaL_checklstring(L, 2, &len);
	SQLSMALLINT params;

	if (odbc->stmt) {
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		odbc->stmt = NULL;
	}

	ClearStatement(odbc);

	if (SQLAllocHandle(SQL_HANDLE_STMT, odbc->dbc, &odbc->stmt) != SQL_SUCCESS) {

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
	else if (SQLNumParams(odbc->stmt, &params)) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}

	odbc->numbparams = (unsigned int)params;

	if (odbc->numbparams > 0) {
		odbc->params = (void**)gff_calloc(odbc->numbparams, sizeof(void*));

		if (!odbc->params) {
			luaL_error(L, "Unable to allocate memory for parameters");
		}
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, true);

	odbc->paramnumber = 0;

	return 1;
}

int ODBCBind(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);
	bool asBinary = lua_toboolean(L, 3) > 0;

	if (!odbc->stmt) {
		luaL_error(L, "No statement is prepared, run Prepare first");
	}
	else if (odbc->paramnumber >= odbc->numbparams) {
		luaL_error(L, "Too many parameters");
	}

	SQLSMALLINT valuetype;
	SQLSMALLINT paramtype;
	SQLSMALLINT decimal = 0;
	SQLUINTEGER len = 0;
	SQLPOINTER data;
	SQLDOUBLE ddata;
	SQLINTEGER idata;
	LuaWChar* wchar;

	size_t rawlen;
	const char* cdata;

	switch (lua_type(L, 2)) {

	case LUA_TNIL:
	case LUA_TNONE:

		valuetype = SQL_C_DEFAULT;
		paramtype = SQL_NULL_DATA;
		len = 0;
		data = NULL;
		break;

	case LUA_TBOOLEAN:

		odbc->params[odbc->paramnumber] = gff_calloc(1, sizeof(SQLINTEGER));
		idata = (SQLINTEGER)lua_toboolean(L, 2);

		if (!odbc->params[odbc->paramnumber]) {
			luaL_error(L, "Failed to allocate memory");
		}

		memcpy(odbc->params[odbc->paramnumber], &idata, sizeof(SQLINTEGER));

		valuetype = SQL_C_SLONG;
		paramtype = SQL_INTEGER;
		len = sizeof(SQLINTEGER);
		data = odbc->params[odbc->paramnumber];

		break;

	case LUA_TNUMBER:

		ddata = (SQLDOUBLE)lua_tonumber(L, 2);
		idata = (SQLINTEGER)lua_tointeger(L, 2);

		if (idata == (SQLINTEGER)ceil(ddata)) {

			odbc->params[odbc->paramnumber] = gff_calloc(1, sizeof(SQLINTEGER));

			if (!odbc->params[odbc->paramnumber]) {
				luaL_error(L, "Failed to allocate memory");
			}

			memcpy(odbc->params[odbc->paramnumber], &idata, sizeof(SQLINTEGER));

			valuetype = SQL_C_SLONG;
			paramtype = SQL_INTEGER;
			len = sizeof(SQLINTEGER);
			data = odbc->params[odbc->paramnumber];
		}
		else {

			odbc->params[odbc->paramnumber] = gff_calloc(1, sizeof(SQLDOUBLE));

			if (!odbc->params[odbc->paramnumber]) {
				luaL_error(L, "Failed to allocate memory");
			}

			memcpy(odbc->params[odbc->paramnumber], &ddata, sizeof(SQLDOUBLE));

			valuetype = SQL_C_DOUBLE;
			paramtype = SQL_DOUBLE;
			len = sizeof(SQLDOUBLE);
			data = odbc->params[odbc->paramnumber];
		}
		break;
	case LUA_TUSERDATA:
		wchar = (LuaWChar*)luaL_checkudata(L, 2, LUAWCHAR);

		if (wchar) {

			odbc->params[odbc->paramnumber] = gff_calloc(wchar->len + 1, sizeof(wchar_t));

			if (!odbc->params[odbc->paramnumber]) {
				luaL_error(L, "Failed to allocate memory");
			}

			memcpy(odbc->params[odbc->paramnumber], wchar->str, wchar->len * sizeof(wchar_t));

			valuetype = SQL_C_WCHAR;
			paramtype = SQL_CHAR;

			data = (SQLPOINTER)odbc->params[odbc->paramnumber];
			len = wchar->len * sizeof(wchar_t);

			break;
		}
		// Fall through

	default:

		cdata = luaL_tolstring(L, 2, &rawlen);

		odbc->params[odbc->paramnumber] = gff_calloc(rawlen + 1, sizeof(char));

		if (!odbc->params[odbc->paramnumber]) {
			luaL_error(L, "Failed to allocate memory");
		}

		memcpy(odbc->params[odbc->paramnumber], cdata, rawlen);

		valuetype = SQL_C_CHAR;
		paramtype = SQL_CHAR;

		data = (SQLPOINTER)odbc->params[odbc->paramnumber];
		len = rawlen;

		break;
	}

	if (asBinary) {
		valuetype = SQL_C_DEFAULT;
		paramtype = SQL_BINARY;
	}

	if (SQLBindParameter(odbc->stmt, ++odbc->paramnumber, SQL_PARAM_INPUT, valuetype, paramtype, len, decimal, data, len + 1, NULL) != SQL_SUCCESS) {

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

int ODBCProcedureColumns(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);
	size_t proclen;
	SQLCHAR* procedure = (SQLCHAR*)luaL_optlstring(L, 2, NULL, &proclen);

	size_t schemalen;
	SQLCHAR* schema = (SQLCHAR*)luaL_optlstring(L, 3, NULL, &schemalen);

	ClearStatement(odbc);

	if (SQLAllocHandle(SQL_HANDLE_STMT, odbc->dbc, &odbc->stmt) != SQL_SUCCESS) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}
	else if (!SUCCEEDED(SQLProcedureColumns(odbc->stmt, NULL, 0, schema, (SQLSMALLINT)schemalen, procedure, (SQLSMALLINT)proclen, NULL, 0))) {

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

int ODBCProcedures(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);
	size_t schemalen;
	SQLCHAR* schema = (SQLCHAR*)luaL_optlstring(L, 2, NULL, &schemalen);

	ClearStatement(odbc);

	if (SQLAllocHandle(SQL_HANDLE_STMT, odbc->dbc, &odbc->stmt) != SQL_SUCCESS) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}
	else if (!SUCCEEDED(SQLProcedures(odbc->stmt, NULL, 0, schema, (SQLSMALLINT)schemalen, NULL, 0))) {

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

int ODBCForeignKeys(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);
	size_t len;
	SQLCHAR* table = (SQLCHAR*)luaL_tolstring(L, 2, &len);
	size_t schemalen;
	SQLCHAR* schema = (SQLCHAR*)luaL_optlstring(L, 3, NULL, &schemalen);

	ClearStatement(odbc);

	if (SQLAllocHandle(SQL_HANDLE_STMT, odbc->dbc, &odbc->stmt) != SQL_SUCCESS) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}
	else if (!SUCCEEDED(SQLForeignKeys(odbc->stmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0, schema, (SQLSMALLINT)schemalen, table, (SQLSMALLINT)len))) {

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

int ODBCPrimaryKeys(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);
	size_t len;
	SQLCHAR* table = (SQLCHAR*)luaL_tolstring(L, 2, &len);
	size_t schemalen;
	SQLCHAR* schema = (SQLCHAR*)luaL_optlstring(L, 3, NULL, &schemalen);


	ClearStatement(odbc);

	if (SQLAllocHandle(SQL_HANDLE_STMT, odbc->dbc, &odbc->stmt) != SQL_SUCCESS) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}
	else if (!SUCCEEDED(SQLPrimaryKeys(odbc->stmt, NULL, 0, schema, (SQLSMALLINT)schemalen, table, (SQLSMALLINT)len))) {

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

int ODBCSpecialColumns(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);
	size_t len;
	SQLCHAR* table = (SQLCHAR*)luaL_optlstring(L, 2, NULL, &len);

	size_t schemalen;
	SQLCHAR* schema = (SQLCHAR*)luaL_optlstring(L, 3, NULL, &schemalen);

	ClearStatement(odbc);

	if (SQLAllocHandle(SQL_HANDLE_STMT, odbc->dbc, &odbc->stmt) != SQL_SUCCESS) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}
	else if (!SUCCEEDED(SQLSpecialColumns(odbc->stmt, SQL_BEST_ROWID, NULL, 0, schema, (SQLSMALLINT)schemalen, table, (SQLSMALLINT)len, SQL_SCOPE_SESSION, SQL_NULLABLE))) {

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

int ODBCColumns(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);
	size_t len;
	SQLCHAR* table = (SQLCHAR*)luaL_optlstring(L, 2, NULL, &len);

	size_t schemalen;
	SQLCHAR* schema = (SQLCHAR*)luaL_optlstring(L, 3, NULL, &schemalen);

	ClearStatement(odbc);

	if (SQLAllocHandle(SQL_HANDLE_STMT, odbc->dbc, &odbc->stmt) != SQL_SUCCESS) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}
	else if (!SUCCEEDED(SQLColumns(odbc->stmt, NULL, 0, schema, (SQLSMALLINT)schemalen, table, (SQLSMALLINT)len, NULL, 0))) {

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

int ODBCTables(lua_State* L) {

	LuaOdbc* odbc = AssertIsOpen(L, 1);

	size_t schemalen;
	SQLCHAR* schema = (SQLCHAR*)luaL_optlstring(L, 2, NULL, &schemalen);

	ClearStatement(odbc);

	if (SQLAllocHandle(SQL_HANDLE_STMT, odbc->dbc, &odbc->stmt) != SQL_SUCCESS) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}
	else if (!SUCCEEDED(SQLTables(odbc->stmt, NULL, 0, schema, (SQLSMALLINT)schemalen, NULL, 0, NULL, 0))) {

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
	else if (odbc->numbparams != odbc->paramnumber) {
		luaL_error(L, "Number of bound parameters does not match expected parameters");
	}

	int result = 1;

	if (!SUCCEEDED(SQLExecute(odbc->stmt))) {

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		result = 2;
	}
	else {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, true);
	}

	if (odbc->params) {

		for (unsigned int i = 0; i < odbc->numbparams; i++)
		{
			if (odbc->params[i]) {
				gff_free(odbc->params[i]);
				odbc->params[i] = NULL;
			}
		}

		memset(odbc->params, 0, sizeof(void*) * odbc->numbparams);
		odbc->paramnumber = 0;
	}

	return result;
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

	if (!SUCCEEDED(SQLNumResultCols(odbc->stmt, &cols))) {
		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, false);

		PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
		SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
		return 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_createtable(L, 0, cols);
	for (SQLSMALLINT i = 0; i < cols; i++)
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
	for (SQLSMALLINT i = 0; i < cols; i++)
	{
		if (!SUCCEEDED(SQLDescribeCol(odbc->stmt, i + 1, columnname, sizeof(columnname), &columnnamesize, &datatype, &columnsize, &decimaldigits, &nullable))) {
			lua_pop(L, lua_gettop(L));
			lua_pushboolean(L, false);

			PushDiagonstics(L, SQL_HANDLE_STMT, odbc->stmt);
			SQLFreeHandle(SQL_HANDLE_STMT, odbc->stmt);
			return 2;
		}

		lua_pushlstring(L, (const char*)columnname, columnnamesize);

		if (datatype == SQL_DECIMAL || datatype == SQL_NUMERIC || datatype == SQL_REAL || datatype == SQL_FLOAT || datatype == SQL_DOUBLE) {

			data = (SQLPOINTER)gff_calloc(1, sizeof(lua_Number));

			if (!data) {
				luaL_error(L, "Unable to allocate memory");
			}

			if (!SUCCEEDED(SQLGetData(odbc->stmt, i + 1, SQL_C_DOUBLE, data, sizeof(lua_Number), &datalen))) {

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
		else if (datatype == SQL_SMALLINT || datatype == SQL_INTEGER || datatype == SQL_TINYINT || datatype == SQL_BIGINT) {

			data = (SQLPOINTER)gff_calloc(1, sizeof(lua_Integer));

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

			data = gff_calloc(sizeof(SQLWCHAR), columnsize);

			if (!data) {
				luaL_error(L, "Unable to allocate memory");
			}

			if (!SUCCEEDED(SQLGetData(odbc->stmt, i + 1, SQL_WCHAR, data, columnsize * sizeof(wchar_t), &datalen))) {

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
				lua_pushwchar(L, (wchar_t*)data, datalen / sizeof(wchar_t));
			}
		}

		if (data) {
			gff_free(data);
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

	LuaWChar* connectionStringw = lua_stringtowchar(L, 1);

	if (!connectionStringw || connectionStringw->len <= 0) {
		luaL_error(L, "Connection string cannot be empty");
	}

	wchar_t* currentconnectionstring = (wchar_t*)gff_calloc(connectionStringw->len + 1, sizeof(wchar_t));
	if (!currentconnectionstring) {
		luaL_error(L, "Failed to allocate memory");
	}
	memcpy(currentconnectionstring, connectionStringw->str, connectionStringw->len * sizeof(wchar_t));

	lua_pop(L, lua_gettop(L));

	LuaOdbc* odbc = lua_pushodbc(L);

	if (!odbc) {
		gff_free(currentconnectionstring);
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
	else if (SQLDriverConnectW(odbc->dbc, NULL, (SQLWCHAR*)odbc->ConnectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE) != SQL_SUCCESS) {

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
		gff_free(odbc->ConnectionString);
		odbc->ConnectionString = NULL;
	}

	ClearStatement(odbc);

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