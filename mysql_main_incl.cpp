#include "mysql_main_incl.h"

void pushmysqlfield(MYSQL_ROW row, MYSQL_FIELD * columns, lua_State *L, int n, unsigned long length, bool asstring) {

	LUA_NUMBER number;
	LUA_INTEGER integer;
	char * temp;

	if (row[n] == NULL) {
		lua_pushnil(L);
	}
	else if (asstring) {
		lua_pushlstring(L, row[n], length);
	}
	else {
		switch (columns[n].type) {

		case MYSQL_TYPE_BIT:

			if (length <= 0) {
				lua_pushboolean(L, false);
			}
			else {
				lua_pushboolean(L, row[n][0] != '0');
			}

			break;
		case MYSQL_TYPE_TINY:
		case MYSQL_TYPE_SHORT:
		case MYSQL_TYPE_LONG:
		case MYSQL_TYPE_LONGLONG:
		case MYSQL_TYPE_INT24:

			if (length <= 0) {
				lua_pushinteger(L, 0);
			}
			else {
				temp = (char*)calloc(length + 1, sizeof(char));

				if (!temp) {
					lua_pushinteger(L, 0);
				}
				else {

					memcpy(temp, row[n], length);
					sscanf_s(temp, "%lld", &integer);
					free(temp);
					lua_pushinteger(L, integer);
				}
			}

			break;
		case MYSQL_TYPE_FLOAT:
		case MYSQL_TYPE_DOUBLE:
		case MYSQL_TYPE_DECIMAL:
		case MYSQL_TYPE_NEWDECIMAL:

			if (length <= 0) {
				lua_pushnumber(L, 0);
			}
			else {
				temp = (char*)calloc(length + 1, sizeof(char));

				if (!temp) {
					lua_pushnumber(L, 0);
				}
				else {

					memcpy(temp, row[n], length);
					sscanf_s(temp, "%lf", &integer);
					free(temp);
					lua_pushnumber(L, integer);
				}
			}
			break;
		default:

			if (length <= 0) {
				lua_pushstring(L, "");
			}
			else {
				lua_pushlstring(L, row[n], length);
			}
			break;
		}
	}
}