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

#define HI_PART(x)  ((x>>4) & 0x0F)
#define LO_PART(x)  ((x) & 0x0F)

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


static int L_kbhit(lua_State *L){

	lua_pushboolean(L, _kbhit());
	return 1;
}

static int L_getch(lua_State *L){
	lua_pushinteger(L, _getch());
	return 1;
}

static int L_GetTextColor(lua_State *L){

	WORD data;
	CONSOLE_SCREEN_BUFFER_INFO   csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)){
		data = csbi.wAttributes;

		lua_pushinteger(L, HI_PART(data));
		lua_pushinteger(L, LO_PART(data));
	}
	else{
		lua_pushnil(L);
		lua_pushnil(L);
	}

	return 2;
}

static int L_SetTextColor(lua_State *L){

	int BackC = luaL_checknumber(L, 1);
	int ForgC = luaL_checknumber(L, 2);

	lua_pop(L, 2);

	WORD wColor = ((BackC & 0x0F) << 4) + (ForgC & 0x0F);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), wColor);

	return 0;
}

static int L_cls(lua_State *L) {

	HANDLE                     hStdOut;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD                      count;
	DWORD                      cellCount;
	COORD                      homeCoords = { 0, 0 };

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE) return 0;

	if (!GetConsoleScreenBufferInfo(hStdOut, &csbi)) return 0;
	cellCount = csbi.dwSize.X *csbi.dwSize.Y;

	if (!FillConsoleOutputCharacter(
		hStdOut,
		(TCHAR) ' ',
		cellCount,
		homeCoords,
		&count
		)) return 0;

	if (!FillConsoleOutputAttribute(
		hStdOut,
		csbi.wAttributes,
		cellCount,
		homeCoords,
		&count
		)) return 0;

	SetConsoleCursorPosition(hStdOut, homeCoords);

	return 0;
}

static int L_put(lua_State *L) {

	size_t len;
	const char * text = luaL_tolstring(L, 1, &len);
	unsigned int n;

	if (len > 0) {

		for (n = 0; n < len; n++) {

			if (text[n] == 13) {
				printf("\n");
			}
			else if (text[n] == 8) {
				printf("\b \b");
			}
			else
				printf("%c", text[n]);
		}
	}

	lua_pop(L, 1);
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

	lua_pushcfunction(L, L_cls);
	lua_setglobal(L, "CLS");

	lua_pushcfunction(L, L_GetTextColor);
	lua_setglobal(L, "GetTextColor");

	lua_pushcfunction(L, L_SetTextColor);
	lua_setglobal(L, "SetTextColor");

	lua_pushcfunction(L, L_getch);
	lua_setglobal(L, "GetKey");

	lua_pushcfunction(L, L_kbhit);
	lua_setglobal(L, "HasKeyDown");

	lua_pushcfunction(L, L_put);
	lua_setglobal(L, "Put");

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