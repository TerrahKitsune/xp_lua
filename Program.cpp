#define _CRT_SECURE_NO_WARNINGS
#include "lua_main_incl.h" 
#include <conio.h>
#include "GFFMain.h"
#include <windows.h>
#include "TimerMain.h"
#include "MySQLMain.h"
#include "lua_misc.h"
#include "LuaFileSystemMain.h"
#include "LuaSQLiteMain.h"

double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCounter()
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		puts("QueryPerformanceFrequency failed!");

	PCFreq = double(li.QuadPart) / 1000.0;

	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;
}
double GetCounter()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - CounterStart) / PCFreq;
}

int GetResults(lua_State *L){

	int cnt = 0;

	for (int n = 1; n < lua_gettop(L); n++){
		if (lua_isnil(L, n))
			break;
		else
			cnt++;
	}

	return cnt;
}

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

void main(int argc, char *argv[]){

	StartCounter();
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);

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

	//returns nothing
	luaopen_misc(L);

	const char * file = "main.lua";
	if (argc>1){
		file = argv[1];
	}

	//lua_pushcfunction(L, print);
	//lua_setglobal(L, "print");

	if (luaL_loadfile(L, file) != 0){
		puts(lua_tostring(L, 1));
		lua_pop(L, 1);
	}
	else if (lua_pcall(L, 0, 10, NULL) != 0){
		puts(lua_tostring(L, 1));
		lua_pop(L, 1);
	}
	else if (GetResults(L) <= 0){
		puts("Script returned 0 results");
	}
	else{
		printf("Script returned %d results\n", GetResults(L));
		DumpStack(L, true);
	}

	lua_pop(L, lua_gettop(L));

	printf("%f", GetCounter());

	lua_close(L);

	_getch();
}