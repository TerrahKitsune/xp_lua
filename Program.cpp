#define _CRT_SECURE_NO_WARNINGS

#include "lua_main_incl.h" 
#include "networking.h"
#include <conio.h>
#include "GFFMain.h"
#include <windows.h>
#include "TimerMain.h"
#include "MySQLMain.h"
#include "lua_misc.h"
#include "LuaFileSystemMain.h"
#include "LuaSQLiteMain.h"
#include "ERFMain.h"
#include "MD5Main.h"
#include "HttpMain.h"
#include "ProcessMain.h"
#include "Http.h"
#include "Shellapi.h"
#include "LuaClientMain.h"
#include "LuaServerMain.h"
#include "TlkMain.h"
#include "2DAMain.h"
#include "ZIPMain.h"
#include "NamedPipeMain.h"
#include <io.h>
#include "LuaImageMain.h"
#include "StreamMain.h"
#include "ODBCMain.h"
#include "WinServicesMain.h"
#include "luasocketmain.h"
#include "lualinkedlistmain.h"
#include "luakafkamain.h"
#include "Sha256Main.h"
#include "LuaFTPMain.h"
#include "FileAsyncMain.h"
#include "LuaMutexMain.h"
#include "LuaAesMain.h"

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

double TickPCFreq = 0.0;
__int64 TickCounterStart = 0;

void TickStartCounter()
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		puts("QueryPerformanceFrequency failed!");

	TickPCFreq = double(li.QuadPart) / 1000.0;

	QueryPerformanceCounter(&li);
	TickCounterStart = li.QuadPart;
}

double TickGetCounter()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - TickCounterStart) / PCFreq;
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

	if (_isatty(_fileno(stdin))) {
		lua_pushboolean(L, _kbhit());
	}
	else {
		lua_pushboolean(L, !feof(stdin));
	}

	return 1;
}

static int L_getch(lua_State *L){

	if (_isatty(_fileno(stdin))) {
		lua_pushinteger(L, _getch());
	}
	else {

		char data;
		if (fread(&data, sizeof(char), 1, stdin) == 1) {
			lua_pushinteger(L, data);
		}
		else {
			lua_pushinteger(L, -1);
		}
	}
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

const char * ReadStdIn(char * buf, size_t bufsize) {
	if (_isatty(_fileno(stdin))) {
		return gets_s(buf, bufsize);
	}
	else {
		size_t size = fread(buf, 1, bufsize-1, stdin);
		if (size <= 0) {
			return NULL;
		}
		buf[size] = '\0';
		return buf;
	}
}

static int L_SetTextColor(lua_State *L){

	int BackC = (int)luaL_checknumber(L, 1);
	int ForgC = (int)luaL_checknumber(L, 2);

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

static int L_Exit(lua_State *L){

	int ExitCode = (int)luaL_optinteger(L, 1, 0);
	lua_pop(L, lua_gettop(L));
	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);
	ERR_free_strings();
	EVP_cleanup();
	WSACleanup();
	exit(ExitCode);
}

static int L_GetMemory(lua_State *L){

	lua_pop(L, lua_gettop(L));
	int mem = lua_gc(L, LUA_GCCOUNT, 0);
	mem = mem * 1024;
	mem += lua_gc(L, LUA_GCCOUNTB, 0);
	lua_pushinteger(L, mem);
	return 1;
}

static int L_ShellExecute(lua_State *L){

	int ok = (int)ShellExecute(NULL, "open", luaL_checkstring(L, 1), luaL_checkstring(L, 2), NULL, SW_SHOW);
	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, ok > 32);
	return 1;
}

static int hook;
static double ticktime;
void L_Ticker(lua_State *L, lua_Debug *ar){
	if (hook == -1)
		return;
	else if (TickGetCounter() < ticktime){
		return;
	}
	else
		TickStartCounter();

	lua_rawgeti(L, LUA_REGISTRYINDEX, hook);
	if (lua_isfunction(L, 1)){
		lua_pcall(L, 0, 0, NULL);
	}
	lua_pop(L, lua_gettop(L));
}

static int L_SetTick(lua_State *L){

	if (!lua_isfunction(L, 1)){
		lua_sethook(L, L_Ticker, 0, 0);
		if (hook != -1)
			luaL_unref(L, LUA_REGISTRYINDEX, hook);
		hook = -1;
		return 0;
	}

	lua_pushvalue(L, 1);
	hook = luaL_ref(L, LUA_REGISTRYINDEX);
	ticktime = luaL_optnumber(L, 2, 1000.0);
	if (ticktime < 0.0)
		ticktime = 0.0;
	lua_pop(L, lua_gettop(L));
	TickStartCounter();

	int maskcnt = (int)ticktime;
	if (maskcnt <= 0)
		maskcnt = 1;
	else if (maskcnt > 1000)
		maskcnt = 1000;

	lua_sethook(L, L_Ticker, 0xFFFFFFFF, maskcnt);

	return 0;
}

static int L_GetReg(lua_State *L){

	HKEY key = HKEY_LOCAL_MACHINE;

	switch (lua_tointeger(L, 1)){
	case 1:
		key = HKEY_CLASSES_ROOT;
		break;
	case 2:
		key = HKEY_CURRENT_CONFIG;
		break;
	case 3:
		key = HKEY_CURRENT_USER;
		break;
	case 4:
		key = HKEY_PERFORMANCE_DATA;
		break;
	case 5:
		key = HKEY_PERFORMANCE_NLSTEXT;
		break;
	case 6:
		key = HKEY_PERFORMANCE_TEXT;
		break;
	case 7:
		key = HKEY_USERS;
		break;
	default:
		break;
	}

	DWORD max = 1048576;
	char * buffer = (char*)malloc(max);
	if (!buffer)
		luaL_error(L, "Unable to allocate memory for readbuffer in GetReg");
	memset(buffer, 0, max);
	LSTATUS status = RegGetValue(key, luaL_checkstring(L, 2), luaL_checkstring(L, 3), RRF_RT_ANY, nullptr, buffer, &max);
	if (status == ERROR_SUCCESS){
		lua_pop(L, lua_gettop(L));
		lua_pushstring(L, buffer);
		free(buffer);
	}
	else{

		lua_pop(L, lua_gettop(L));

		char *err;
		if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			status,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
			(LPTSTR)&err,
			0,
			NULL))
		{
			lua_pushnil(L);
			lua_pushstring(L, "unable to format error message!");
		}
		else{
			lua_pushnil(L);
			lua_pushstring(L, err);
			LocalFree(err);
		}	
		free(buffer);

		return 2;
	}
	return 1;
}

static int L_ToggleConsole(lua_State *L){

	bool toggle = lua_toboolean(L, 1) > 0;
	HWND console = GetConsoleWindow();
	if (toggle){		
		ShowWindow(console, SW_RESTORE);
	}
	else{
		ShowWindow(console, SW_HIDE);
	}
	lua_pop(L, 1);
	return 0;
}

static int L_SetTitle(lua_State *L){
	SetConsoleTitle(luaL_checkstring(L, 1));
	lua_pop(L, 1);
	return 0;
}

static int L_GetRuntime(lua_State *L){
	lua_pushnumber(L, GetCounter());
	return 1;
}

int main(int argc, char *argv[]){

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return -1;
	}

	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();

	StartCounter();
	hook = -1;
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);

#ifdef _DEBUG
	lua_pushboolean(L, TRUE);
	lua_setglobal(L, "DEBUG");
#endif

	lua_createtable(L, 0, argc);
	for (int n = 0; n < argc; n++){
		lua_pushstring(L, argv[n]);
		lua_rawseti(L, -2, n + 1);
	}
	lua_setglobal(L, "ARGS");

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
	luaopen_namedpipe(L);
	lua_setglobal(L, "Pipe");
	luaopen_image(L);
	lua_setglobal(L, "Image");
	luaopen_stream(L);
	lua_setglobal(L, "Stream");
	luaopen_odbc(L);
	lua_setglobal(L, "ODBC");
	luaopen_winservice(L);
	lua_setglobal(L, "Services");
	luaopen_socket(L);
	lua_setglobal(L, "Socket");
	luaopen_linkedlist(L);
	lua_setglobal(L, "LinkedList");
	luaopen_kafka(L);
	lua_setglobal(L, "Kafka");
	luaopen_sha256(L);
	lua_setglobal(L, "SHA256");
	luaopen_ftp(L);
	lua_setglobal(L, "FTP");
	luaopen_fileasync(L);
	lua_setglobal(L, "FileAsync");
	luaopen_mutex(L);
	lua_setglobal(L, "Mutex");
	luaopen_luaaes(L);
	lua_setglobal(L, "Aes");

	lua_pushcfunction(L, L_GetRuntime);
	lua_setglobal(L, "Runtime");

	lua_pushcfunction(L, L_SetTitle);
	lua_setglobal(L, "SetTitle");

	lua_pushcfunction(L, L_ToggleConsole);
	lua_setglobal(L, "ToggleConsole");

	lua_pushcfunction(L, L_GetReg);
	lua_setglobal(L, "GetRegistryValue");

	lua_pushcfunction(L, L_SetTick);
	lua_setglobal(L, "SetTicker");

	lua_pushcfunction(L, L_ShellExecute);
	lua_setglobal(L, "ShellExecute");

	lua_pushcfunction(L, L_GetMemory);
	lua_setglobal(L, "GetMemory");

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

	lua_pushcfunction(L, L_cls);
	lua_setglobal(L, "CLS");

	lua_pushcfunction(L, L_Exit);
	lua_setglobal(L, "Exit");

	//returns nothing
	luaopen_misc(L);

	const char * file = "main.lua";
	if (argc > 1){
		file = argv[1];
	}

	FILE * exists = NULL;

	if(_stricmp(file, "cmd") != 0)
		exists = fopen(file, "r");
	

	if (exists) {
		fclose(exists);
		SetConsoleTitle(file);

		if (luaL_loadfile(L, file) != 0) {
			puts(lua_tostring(L, 1));
			lua_pop(L, 1);
			_getch();
		}
		else if (lua_pcall(L, 0, 10, NULL) != 0) {
			puts(lua_tostring(L, 1));
			lua_pop(L, 1);
		}
		else if (GetResults(L) <= 0) {
			puts("Script returned 0 results");
		}
		else {
			printf("Script returned %d results\n", GetResults(L));
			DumpStack(L, true);
		}

		int ret = 0;

		if (lua_type(L, 1) == LUA_TNUMBER) {
			ret = (int)lua_tointeger(L, 1);
		}

		lua_pop(L, lua_gettop(L));

		printf("%f", GetCounter());

		luaserver_KillAll(L);

		lua_getglobal(L, "Exit");
		if (lua_type(L, 1) == LUA_TFUNCTION) {
			lua_pushinteger(L, ret);
			lua_pcall(L, 1, 1, NULL);
			if (lua_type(L, 1) == LUA_TNUMBER) {
				ret = (int)lua_tointeger(L, 1);
			}
			else {
				ret = 0;
			}
		}
		else {
			ret = _getch();
		}

		if (lua_isnumber(L, -1)) {
			ret = (int)lua_tointeger(L, -1);
		}
		else {
			ret = 0;
		}

		lua_pop(L, lua_gettop(L));
		lua_pushinteger(L, ret);

		L_Exit(L);

		return ret;
	}
	else {

		char buf[65536];
		SetConsoleTitle("GFF");

		while (ReadStdIn(buf, sizeof(buf)) != NULL) {

			int error = luaL_loadbuffer(L, buf, strlen(buf), "line") ||
				lua_pcall(L, 0, 0, 0);
			if (error) {
				fprintf(stderr, "%s\n", lua_tostring(L, -1));
				fflush(stderr);
				lua_pop(L, 1); 
			}

			lua_pop(L, lua_gettop(L));
		}
	}

	return 0;
}