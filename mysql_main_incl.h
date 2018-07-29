#pragma once
#define _CRT_SECURE_NO_WARNINGS
#pragma warning (disable: 4514 4786)
#pragma warning( push, 3 )
//#define HAVE_STRUCT_TIMESPEC
#include <my_global.h>
#include <mysql.h>
#include "lua_main_incl.h"

void pushmysqlfield(MYSQL_ROW row, MYSQL_FIELD * columns, lua_State *L, int n, unsigned long length, bool asstring);