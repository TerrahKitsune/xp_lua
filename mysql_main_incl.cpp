#include "mysql_main_incl.h"

void pushmysqlfield(MYSQL_ROW row, MYSQL_FIELD * columns, lua_State *L, int n, unsigned long length, bool asstring) {

	LUA_NUMBER number;
	LUA_INTEGER integer;

	if (row[n] == NULL) {
		lua_pushnil(L);
	}
	else if (asstring) {
		lua_pushlstring(L, row[n], length);
	}
	else {
		switch (columns[n].type) {

		case MYSQL_TYPE_BIT:
			lua_pushboolean(L, row[n][0] != '0');
			break;
		case MYSQL_TYPE_TINY:
		case MYSQL_TYPE_SHORT:
		case MYSQL_TYPE_LONG:
		case MYSQL_TYPE_LONGLONG:
		case MYSQL_TYPE_INT24:
			sscanf_s(row[n], "%lld", &integer);
			lua_pushinteger(L, integer);
			break;
		case MYSQL_TYPE_FLOAT:
		case MYSQL_TYPE_DOUBLE:
		case MYSQL_TYPE_DECIMAL:
		case MYSQL_TYPE_NEWDECIMAL:
			sscanf_s(row[n], "%lf", &number);
			lua_pushnumber(L, number);
			break;
		default:
			lua_pushlstring(L, row[n], length);
			break;
		}
	}
}