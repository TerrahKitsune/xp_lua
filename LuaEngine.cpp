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
#include "LuaNWN.h"
#include "ZIPMain.h"
#include "TlkMain.h"
#include "2DAMain.h"
#include "NamedPipeMain.h"

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

#ifdef _DEBUG
	Log = true;
#else
	Log = false;
#endif

	_lasterror = NULL;
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
	luaopen_filesystem(L);
	lua_setglobal(L, "FileSystem");
	luaopen_sqlite(L);
	lua_setglobal(L, "SQLite");
	luaopen_md5(L);
	lua_setglobal(L, "MD5");
	luaopen_erf(L);
	lua_setglobal(L, "ERF");
	luaopen_http(L);
	lua_setglobal(L, "Http");
	luaopen_process(L);
	lua_setglobal(L, "Process");
	luaopen_luaserver(L);
	lua_setglobal(L, "Server");
	luaopen_luaclient(L);
	lua_setglobal(L, "Client");
	luaopen_tlk(L);
	lua_setglobal(L, "TLK");
	luaopen_twoda(L);
	lua_setglobal(L, "TWODA");
	luaopen_zip(L);
	lua_setglobal(L, "Zip");
	luaopen_nwnfunctions(L);
	lua_setglobal(L, "NWN");
	luaopen_namedpipe(L);
	lua_setglobal(L, "Pipe");

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
	SetError(NULL);
}

void LuaEngine::SetError(const char * err, size_t len){

	if (_lasterror){
		delete[]_lasterror;
		_lasterror = NULL;
	}

	if (err){

		if (len <= 0)
			len = strlen(err);

		if (len <= 0)
			return;

		_lasterror = new char[len + 1]; 
		if (!_lasterror)
			return;
		memcpy(_lasterror, err, len);
		_lasterror[len] = '\0';
	}
}

const char * LuaEngine::GetLastError(){
	return _lasterror;
}

char * LuaEngine::RunFunction(const char * function, const char * param1, int param2, const char * value){
	
	SetError(NULL);

	lua_getglobal(L, function);

	if (lua_type(L, -1) != LUA_TFUNCTION){
		SetError("Function not found");
		lua_pop(L, lua_gettop(L));
		return NULL;
	}

	lua_pushstring(L, param1);
	lua_pushinteger(L, param2);
	lua_pushstring(L, value);

	return Luapcall(3);
}

char * LuaEngine::Luapcall(int params){

	int error = lua_pcall(L, params, 1, 0);
	size_t len;
	const char * luaresult;
	char * result;

	if (error)
	{
		luaresult = lua_tolstring(L, -1, &len);

		SetError(luaresult, len);

		lua_pop(L, lua_gettop(L));
		return NULL;
	}
	else if (lua_gettop(L) >= 1 && !lua_isnoneornil(L, -1))
	{
		if (lua_isboolean(L, -1)) {

			if (lua_toboolean(L, -1)) {
				luaresult = "1";
			}
			else {
				luaresult = "0";
			}

			len = 1;
		}
		else {
			luaresult = lua_tolstring(L, -1, &len);
		}

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

char * LuaEngine::RunString(const char * script, const char * name)
{
	SetError(NULL);

	int error = luaL_loadbuffer(L, script, strlen(script), name);
	size_t len;
	const char * luaresult;

	if (error)
	{
		luaresult = lua_tolstring(L, -1, &len);

		SetError(luaresult, len);

		lua_pop(L, lua_gettop(L));
		return NULL;
	}

	return Luapcall(0);
}
