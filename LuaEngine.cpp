#include "LuaEngine.h"
#include "NWN2LuaLib.h"

#include "GFFMain.h"
#include "TimerMain.h"
#include "MySQLMain.h"
#include "LuaSQLiteMain.h"
#include "LuaFileSystemMain.h"
#include "lua_misc.h"
#include "ERFMain.h"
#include "MD5Main.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
static int print(lua_State *L){

	FILE * file = fopen("LUA.txt", "a");
	const char * data;
	size_t len = 0;

	if (file){
		for (int n = 1; n <= lua_gettop(L); n++){
			data = luaL_tolstring(L, n, &len);
			if (data && len > 0){
				fwrite(data, 1, len, file);
				fwrite("\t", 1, 1, file);
				fflush(file);
			}
			lua_pop(L, 1);
		}
		fwrite("\n", 1, 1, file);
		fflush(file);
		fclose(file);
	}
	lua_pop(L, lua_gettop(L));
	return 0;
}

LuaEngine::LuaEngine()
{
	L = luaL_newstate();
	luaL_openlibs(L);
	lua_nwn2_openlib(L);

	luaopen_gff(L);
	lua_setglobal(L, "GFF");
	luaopen_timer(L);
	lua_setglobal(L, "Timer");
	luaopen_mysql(L);
	lua_setglobal(L, "MySQL");
	luaopen_sqlite(L);
	lua_setglobal(L, "SQLite");
	luaopen_filesystem(L);
	lua_setglobal(L, "FileSystem");
	luaopen_erf(L);
	lua_setglobal(L, "ERF");
	luaopen_md5(L);
	lua_setglobal(L, "MD5");

	luaopen_misc(L);

	lua_pushcfunction(L, print);
	lua_setglobal(L, "print");
}


LuaEngine::~LuaEngine()
{
	lua_close(L);
}

char * LuaEngine::RunString(const char * script, const char * name)
{
	int error = luaL_loadbuffer(L, script, strlen(script), name);
	size_t len;
	const char * luaresult;
	char * result;

	if (error)
	{
		luaresult = lua_tolstring(L, -1, &len);
		result = new char[len+2];
		result[0] = '%';
		memcpy(&result[1], luaresult, len);
		result[len]='\0';
		lua_pop(L, lua_gettop(L));
		return result;
	}

	error = lua_pcall(L, 0, 1, 0);

	if (error)
	{
		luaresult = lua_tolstring(L, -1, &len);
		result = new char[len + 2];
		if (!result){
			return NULL;
		}
		result[0] = '%';
		memcpy(&result[1], luaresult, len);
		result[len] = '\0';
		lua_pop(L, lua_gettop(L));
		return result;
	}
	else if (lua_gettop(L) >= 1 && !lua_isnoneornil(L, -1))
	{
		luaresult = lua_tolstring(L, -1, &len);
		result = new char[len + 1];
		if (!result){
			return NULL;
		}
		memcpy(result, luaresult, len);
		result[len] = '\0';
		lua_pop(L, lua_gettop(L));
		return result;
	}
	else
	{
		lua_pop(L, lua_gettop(L));
		return NULL;
	}
}
