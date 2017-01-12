#include "networking.h"
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
#include "ProcessMain.h"
#include "Http.h"
#include "Shellapi.h"
#include "LuaClientMain.h"
#include "LuaServerMain.h"
#include "HttpMain.h"

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
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();


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
	luaopen_http(L);
	lua_setglobal(L, "Http");
	luaopen_process(L);
	lua_setglobal(L, "Process");
	luaopen_luaserver(L);
	lua_setglobal(L, "Server");
	luaopen_luaclient(L);
	lua_setglobal(L, "Client");

	luaopen_misc(L);

	lua_pushcfunction(L, print);
	lua_setglobal(L, "print");
}


LuaEngine::~LuaEngine()
{
	lua_close(L);
	ERR_free_strings();
	EVP_cleanup();
	WSACleanup();
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
