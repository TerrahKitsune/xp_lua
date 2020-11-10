#include "networking.h"
#include "lua_misc.h"
#include <objbase.h>
#include <time.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <conio.h>
#include "lua_json.h"
#include "crc32.h"

#pragma comment (lib , "winmm.lib")

#define HI_PART(x)  ((x>>4) & 0x0F)
#define LO_PART(x)  ((x) & 0x0F)
#define DIV 1024

static int env_table = -1;
static int env_original = -1;

int lua_uuid(lua_State* L) {

	GUID guid;
	if (CoCreateGuid(&guid) != S_OK) {
		lua_pushnil(L);
		return 1;
	}

	char buffer[37];

	sprintf(buffer, "%08lx-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	lua_pushstring(L, buffer);

	unsigned long data1 = _byteswap_ulong(guid.Data1);
	unsigned short data2 = _byteswap_ushort(guid.Data2);
	unsigned short data3 = _byteswap_ushort(guid.Data3);

	memcpy(&buffer[0], &data1, sizeof(data1));
	memcpy(&buffer[sizeof(data1)], &data2, sizeof(data2));
	memcpy(&buffer[sizeof(data1) + sizeof(data2)], &data3, sizeof(data3));
	memcpy(&buffer[sizeof(data1) + sizeof(data2) + sizeof(data3)], guid.Data4, sizeof(guid.Data4));

	lua_pushlstring(L, buffer, sizeof(data1) + sizeof(data2) + sizeof(data3) + sizeof(guid.Data4));

	return 2;
}

int lua_sleep(lua_State* L) {

	int zzz = (int)luaL_optinteger(L, 1, 1);

	if (zzz <= 0)
		zzz = 1;
	else if (zzz > 1000)
		zzz = 1000;

	Sleep(zzz);
	lua_pop(L, 1);
	return 0;
}

static int GetLastErrorAsMessage(lua_State* L)
{
	DWORD lasterror = (DWORD)luaL_optinteger(L, 1, GetLastError());
	char err[1024];

	DWORD ok = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lasterror,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 1024, NULL);

	lua_pop(L, lua_gettop(L));
	lua_pushlstring(L, err, ok);
	lua_pushinteger(L, lasterror);

	return 2;
}

int Time(lua_State* L) {

	//https://gist.github.com/e-yes/278302
	FILETIME ft;
	LARGE_INTEGER li;

	/* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
	* to a LARGE_INTEGER structure. */
	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	LONGLONG ret = li.QuadPart;
	ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
	ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */

	lua_pushinteger(L, ret);

	return 1;
}

int NewEnvironment(lua_State* L) {

	if (!lua_isstring(L, 1) || lua_gettop(L) != 1) {
		luaL_error(L, "Invalid parameters");
		return 0;
	}

	lua_newtable(L);

	lua_rawgeti(L, LUA_REGISTRYINDEX, env_table);

	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	lua_settable(L, -3);

	lua_pushvalue(L, 2);
	lua_copy(L, 2, 1);
	lua_pop(L, 3);

	return 1;
}

int GetEnvironment(lua_State* L) {

	if (!lua_isstring(L, 1) || lua_gettop(L) != 1) {
		luaL_error(L, "Invalid parameters");
		return 0;
	}

	lua_rawgeti(L, LUA_REGISTRYINDEX, env_table);

	lua_pushvalue(L, 1);
	lua_gettable(L, -2);

	lua_copy(L, 3, 1);

	lua_pop(L, 2);

	return 1;
}

int GetCreateEnvironment(lua_State* L) {

	const char* name = luaL_checkstring(L, 1);

	GetEnvironment(L);
	if (lua_istable(L, 1)) {
		return 1;
	}
	else {
		lua_pop(L, lua_gettop(L));
		lua_pushstring(L, name);
		return NewEnvironment(L);
	}
}

int GetAllEnvironment(lua_State* L) {

	lua_rawgeti(L, LUA_REGISTRYINDEX, env_table);

	return 1;
}

int GetStringEqual(lua_State* L) {

	size_t len1;
	size_t len2;
	const char* str1 = lua_tolstring(L, 1, &len1);
	const char* str2 = lua_tolstring(L, 2, &len2);

	lua_pop(L, lua_gettop(L));

	if (!str1 || !str2) {
		lua_pushboolean(L, str1 == str2);
	}
	else if (str1 == str2 && len1 == len2) {
		lua_pushboolean(L, true);
	}
	else if (len1 == len2) {

		for (size_t i = 0; i < len1; i++)
		{
			if (tolower(str1[i]) != tolower(str2[i])) {
				lua_pushboolean(L, false);
				return 1;
			}
		}

		lua_pushboolean(L, true);
	}
	else {
		lua_pushboolean(L, false);
	}

	return 1;
}

int TableFirst(lua_State* L) {

	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	if (lua_gettop(L) > 2) {
		lua_pop(L, lua_gettop(L) - 2);
	}

	lua_pushvalue(L, 1);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		lua_pushvalue(L, 2);
		lua_pushvalue(L, 4);
		lua_pushvalue(L, 5);

		if (lua_pcall(L, 2, 1, 0) != 0) {
			luaL_error(L, lua_tostring(L, -1));
			return 0;
		}

		if (!lua_isnil(L, -1)) {
			lua_copy(L, -1, 1);
			lua_pop(L, lua_gettop(L) - 1);
			return 1;
		}

		lua_pop(L, 2);
	}

	lua_pop(L, lua_gettop(L));
	lua_pushnil(L);
	return 1;
}

int TableSelect(lua_State* L) {

	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	if (lua_gettop(L) > 2) {
		lua_pop(L, lua_gettop(L) - 2);
	}

	int n = 0;
	lua_createtable(L, 0, 0);
	lua_pushvalue(L, 1);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		lua_pushvalue(L, 2);
		lua_pushvalue(L, 5);
		lua_pushvalue(L, 6);

		if (lua_pcall(L, 2, 1, 0) != 0) {
			luaL_error(L, lua_tostring(L, -1));
			return 0;
		}

		if (!lua_isnil(L, -1)) {

			lua_rawseti(L, 3, ++n);
			lua_pop(L, 1);
		}
		else {
			lua_pop(L, 2);
		}
	}

	lua_copy(L, 3, 1);
	lua_pop(L, lua_gettop(L) - 1);

	return 1;
}

int CRC32(lua_State* L) {

	DWORD crc = 0xFFFFFFFF;

	size_t len;
	void* data;
	lua_Number lnumb;
	lua_Integer lint;
	int lbool;

	if (lua_isnumber(L, 1)) {


		if (lua_isinteger(L, 1)) {
			lint = lua_tointeger(L, 1);
			data = &lint;
			len = sizeof(lua_Integer);
		}
		else {
			lnumb = lua_tonumber(L, 1);
			data = &lnumb;
			len = sizeof(lua_Number);
		}
	}
	else if (lua_isboolean(L, 1)) {
		lbool = lua_toboolean(L, 1);
		data = &lbool;
		len = sizeof(int);
	}
	else {
		data = (void*)lua_tolstring(L, 1, &len);
	}

	if (lua_isnumber(L, 2)) {
		crc = ~(DWORD)lua_tonumber(L, 2);
	}

	crc = crc32((BYTE*)data, len, crc);

	lua_pop(L, lua_gettop(L));

	lua_pushinteger(L, crc);

	return 1;
}

int luabeep(lua_State* L) {

	DWORD freq = luaL_checkinteger(L, 1);
	DWORD dur = luaL_checkinteger(L, 2);

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, Beep(freq, dur));

	return 1;
}

int luasound(lua_State* L) {

	const char* sound = NULL;

	if (!lua_isnoneornil(L, 1)) {
		sound = lua_tostring(L, 1);
	}

	DWORD Flags = SND_FILENAME | SND_NODEFAULT;

	if (lua_toboolean(L, 2)) {
		Flags |= SND_ASYNC;
	}
	else {
		Flags |= SND_SYNC;
	}

	lua_pop(L, lua_gettop(L));

	lua_pushboolean(L, PlaySound(sound, NULL, Flags));

	return 1;
}

int luasoundcommand(lua_State* L) {

	const char* cmd = luaL_checkstring(L, 1);
	char retstring[1024] = { 0 };

	MCIERROR result = mciSendString(cmd, retstring, 1024, NULL);

	lua_pop(L, lua_gettop(L));

	lua_pushinteger(L, result);
	lua_pushstring(L, retstring);

	return 2;
}

int setenv(const char* name, const char* value, int overwrite)
{
	int errcode = 0;
	if (!overwrite) {
		size_t envsize = 0;
		errcode = getenv_s(&envsize, NULL, 0, name);
		if (errcode || envsize) return errcode;
	}
	return _putenv_s(name, value);
}

int luasetenv(lua_State* L) {

	const char* var = luaL_checkstring(L, 1);
	const char* value = luaL_checkstring(L, 2);
	bool allowOverwrite = lua_toboolean(L, 3) > 0;

	lua_pop(L, lua_gettop(L));

	lua_pushinteger(L, setenv(var, value, allowOverwrite));

	return 1;
}

int luagetenv(lua_State* L) {

	const char* var = luaL_checkstring(L, 1);

	size_t len;
	int error = getenv_s(&len, NULL, 0, var);

	lua_pop(L, lua_gettop(L));

	if (error) {
		lua_pushnil(L);
		return 1;
	}
	else if (len <= 0) {
		lua_pushstring(L, "");
		return 1;
	}

	char* data = (char*)gff_calloc(sizeof(char), len + 1);

	if (!data) {
		lua_pushnil(L);
		return 1;
	}

	error = getenv_s(&len, data, len, var);

	if (error) {

		gff_free(data);
		lua_pushnil(L);
		return 1;
	}

	lua_pushlstring(L, data, len);

	gff_free(data);

	return 1;
}

static int L_cls(lua_State* L) {

	HANDLE                     hStdOut;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD                      count;
	DWORD                      cellCount;
	COORD                      homeCoords = { 0, 0 };

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE) return 0;

	if (!GetConsoleScreenBufferInfo(hStdOut, &csbi)) return 0;
	cellCount = csbi.dwSize.X * csbi.dwSize.Y;

	if (!FillConsoleOutputCharacter(
		hStdOut,
		(TCHAR)' ',
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

static int L_SetConsoleCoords(lua_State* L) {

	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L, 1);
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE) return 0;

	COORD homeCoords = { x, y };

	SetConsoleCursorPosition(hStdOut, homeCoords);

	return 0;
}

static int L_GetConsoleCoords(lua_State* L) {

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	lua_pop(L, lua_gettop(L));

	if (hStdOut == INVALID_HANDLE_VALUE) return 0;

	if (!GetConsoleScreenBufferInfo(hStdOut, &info)) {

		lua_pushnil(L);
		return 1;
	}

	lua_pushinteger(L, info.dwCursorPosition.X);
	lua_pushinteger(L, info.dwCursorPosition.Y);
	lua_pushinteger(L, info.dwSize.X);
	lua_pushinteger(L, info.dwSize.Y);
	lua_pushinteger(L, info.dwMaximumWindowSize.X);
	lua_pushinteger(L, info.dwMaximumWindowSize.Y);

	return 6;
}

static int L_ConsoleCreate(lua_State* L) {

	lua_pop(L, lua_gettop(L));
	BOOL ok = AllocConsole();

	lua_pushboolean(L, ok > 0);

	return 1;
}

static int L_ConsoleDestroy(lua_State* L) {

	lua_pop(L, lua_gettop(L));
	BOOL ok = FreeConsole();

	lua_pushboolean(L, ok > 0);

	return 1;
}

static int L_SetTitle(lua_State* L) {
	SetConsoleTitle(luaL_checkstring(L, 1));
	lua_pop(L, 1);
	return 0;
}

static int L_ToggleConsole(lua_State* L) {

	bool toggle = lua_toboolean(L, 1) > 0;
	HWND console = GetConsoleWindow();
	if (toggle) {
		ShowWindow(console, SW_RESTORE);
	}
	else {
		ShowWindow(console, SW_HIDE);
	}
	lua_pop(L, 1);
	return 0;
}

static int L_SetTextColor(lua_State* L) {

	int BackC = (int)luaL_checknumber(L, 1);
	int ForgC = (int)luaL_checknumber(L, 2);

	lua_pop(L, 2);

	WORD wColor = ((BackC & 0x0F) << 4) + (ForgC & 0x0F);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), wColor);

	return 0;
}

static int L_GetTextColor(lua_State* L) {

	WORD data;
	CONSOLE_SCREEN_BUFFER_INFO   csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		data = csbi.wAttributes;

		lua_pushinteger(L, HI_PART(data));
		lua_pushinteger(L, LO_PART(data));
	}
	else {
		lua_pushnil(L);
		lua_pushnil(L);
	}

	return 2;
}

static int L_ConsoleWrite(lua_State* L) {

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hStdOut == INVALID_HANDLE_VALUE) {
		return 0;
	}

	size_t len;
	const char* data;

	if (lua_isstring(L, 1)) {
		data = lua_tolstring(L, 1, &len);
	}
	else {
		data = luaL_tolstring(L, 1, &len);
	}

	DWORD written;
	WriteConsole(hStdOut, data, len, &written, NULL);

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, written);

	return 1;
}

static int L_ConsolePrint(lua_State* L) {

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hStdOut == INVALID_HANDLE_VALUE) {
		return 0;
	}

	DWORD written;
	DWORD total = 0;
	size_t len;
	const char* data;

	for (int n = 1; n <= lua_gettop(L); n++) {

		data = luaL_tolstring(L, n, &len);
		lua_pop(L, 1);

		if (!data) {
			data = "";
			len = 0;
		}

		WriteConsole(hStdOut, data, len, &written, NULL);
		total += written;

		if (n < lua_gettop(L)) {
			data = "\t";
			len = 1;
			WriteConsole(hStdOut, data, len, &written, NULL);
			total += written;
		}
		else {
			data = "\n";
			len = 1;
			WriteConsole(hStdOut, data, len, &written, NULL);
			total += written;
		}
	}

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, total);

	return 1;
}

static int L_ConsoleReadKey(lua_State* L) {

	HANDLE hStdOut = GetStdHandle(STD_INPUT_HANDLE);

	if (hStdOut == INVALID_HANDLE_VALUE) {
		return 0;
	}

	lua_pop(L, lua_gettop(L));

	bool keydown = false;

	if (_isatty(_fileno(stdin))) {
		keydown = _kbhit() > 0;
	}
	else {
		keydown = !feof(stdin);
	}

	if (keydown) {
		lua_pushinteger(L, _getch());
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

static int L_GetHost(lua_State* L) {

	const char* data = lua_tostring(L, 1);
	struct addrinfo* result = NULL, * ptr = NULL, hints;

	bool full = lua_toboolean(L, 2) > 0;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = 0;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_ALL;

	int iResult = getaddrinfo(data, NULL, &hints, &result);

	lua_pop(L, lua_gettop(L));
	int n = 0;

	if (iResult == 0) {

		if (full) {
			lua_newtable(L);
		}

		for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

			if (full && (ptr->ai_family == AF_INET || ptr->ai_family == AF_INET6)) {

				lua_newtable(L);

				lua_pushstring(L, "Type");

				if (ptr->ai_family == AF_INET) {
					lua_pushstring(L, "IPV4");
				}
				else {
					lua_pushstring(L, "IPV6");
				}

				lua_settable(L, -3);

				if (ptr->ai_family == AF_INET) {

					struct sockaddr_in* b = (struct sockaddr_in*)ptr->ai_addr;
					char straddr[INET_ADDRSTRLEN];

					lua_pushstring(L, "IP");
					lua_pushstring(L, inet_ntop(AF_INET, &b->sin_addr, straddr, sizeof(straddr)));
					lua_settable(L, -3);
				}
				else {

					struct sockaddr_in6* a = (struct sockaddr_in6*)ptr->ai_addr;
					char straddr[INET6_ADDRSTRLEN];

					lua_pushstring(L, "IP");
					lua_pushstring(L, inet_ntop(AF_INET6, &a->sin6_addr, straddr, sizeof(straddr)));
					lua_settable(L, -3);
				}

				lua_rawseti(L, -2, ++n);
			}
			else if (!full && ptr->ai_family == AF_INET) {

				struct sockaddr_in* b = (struct sockaddr_in*)ptr->ai_addr;
				char straddr[INET_ADDRSTRLEN];

				lua_pushstring(L, inet_ntop(AF_INET, &b->sin_addr, straddr, sizeof(straddr)));

				freeaddrinfo(result);

				return 1;
			}
		}

		freeaddrinfo(result);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

static int L_AttachConsole(lua_State* L) {

	DWORD processId = luaL_optinteger(L, 1, ATTACH_PARENT_PROCESS);

	lua_pop(L, lua_gettop(L));
	BOOL ok = AttachConsole(processId);

	lua_pushboolean(L, ok > 0);

	return 1;
}

static int L_GetComputerName(lua_State* L) {

	char data[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD len;

	lua_pop(L, lua_gettop(L));

	if (GetComputerNameEx(ComputerNameDnsFullyQualified, data, &len)) {

		lua_pushlstring(L, data, len);
	}
	else {

		lua_pushnil(L);
	}

	return 1;
}

int L_GetGlobalMemoryStatus(lua_State* L) {

	int type = luaL_optinteger(L, 1, 0);

	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);

	switch (type)
	{
	case 1:
		lua_pushinteger(L, statex.ullTotalPhys / DIV);
		break;

	case 2:
		lua_pushinteger(L, statex.ullAvailPhys / DIV);
		break;

	case 3:
		lua_pushinteger(L, statex.ullTotalPageFile / DIV);
		break;

	case 4:
		lua_pushinteger(L, statex.ullAvailPageFile / DIV);
		break;

	case 5:
		lua_pushinteger(L, statex.ullTotalVirtual / DIV);
		break;

	case 6:
		lua_pushinteger(L, statex.ullAvailVirtual / DIV);
		break;

	default:
		lua_pushinteger(L, statex.dwMemoryLoad);
		break;
	}

	return 1;
}

int L_DebugBreak(lua_State* L) {

	DebugBreak();

	return 0;
}

int luaopen_misc(lua_State* L) {

	lua_newtable(L);
	env_table = luaL_ref(L, LUA_REGISTRYINDEX);

	char esc[2] = { 0,0 };

	lua_createtable(L, 0, 3);

	lua_pushstring(L, "NUL");
	lua_pushlstring(L, esc, 1);
	lua_settable(L, -3);

	for (esc[0] = 1; esc[0] < 32; esc[0]++) {

		switch (esc[0]) {

		case 1:
			lua_pushstring(L, "SOH");
			break;
		case 2:
			lua_pushstring(L, "STX");
			break;
		case 3:
			lua_pushstring(L, "ETX");
			break;
		case 4:
			lua_pushstring(L, "EOT");
			break;
		case 5:
			lua_pushstring(L, "ENQ");
			break;
		case 6:
			lua_pushstring(L, "ACK");
			break;
		case 7:
			lua_pushstring(L, "BEL");
			break;
		case 8:
			lua_pushstring(L, "BS");
			break;
		case 9:
			lua_pushstring(L, "TAB");
			break;
		case 10:
			lua_pushstring(L, "LF");
			break;
		case 11:
			lua_pushstring(L, "VT");
			break;
		case 12:
			lua_pushstring(L, "FF");
			break;
		case 13:
			lua_pushstring(L, "CR");
			break;
		case 14:
			lua_pushstring(L, "SO");
			break;
		case 15:
			lua_pushstring(L, "SI");
			break;
		case 16:
			lua_pushstring(L, "DLE");
			break;
		case 17:
			lua_pushstring(L, "DC1");
			break;
		case 18:
			lua_pushstring(L, "DC2");
			break;
		case 19:
			lua_pushstring(L, "DC3");
			break;
		case 20:
			lua_pushstring(L, "DC4");
			break;
		case 21:
			lua_pushstring(L, "NAK");
			break;
		case 22:
			lua_pushstring(L, "SYN");
			break;
		case 23:
			lua_pushstring(L, "ETB");
			break;
		case 24:
			lua_pushstring(L, "CAN");
			break;
		case 25:
			lua_pushstring(L, "EM");
			break;
		case 26:
			lua_pushstring(L, "SUB");
			break;
		case 27:
			lua_pushstring(L, "ESC");
			break;
		case 28:
			lua_pushstring(L, "FS");
			break;
		case 29:
			lua_pushstring(L, "GS");
			break;
		case 30:
			lua_pushstring(L, "RS");
			break;
		case 31:
			lua_pushstring(L, "US");
			break;
		case 32:
			lua_pushstring(L, "Space");
			break;
		}

		lua_pushstring(L, esc);
		lua_settable(L, -3);
	}

	lua_setglobal(L, "c");

	lua_newtable(L);

	lua_pushstring(L, "Attach");
	lua_pushcfunction(L, L_AttachConsole);
	lua_settable(L, -3);

	lua_pushstring(L, "Print");
	lua_pushcfunction(L, L_ConsolePrint);
	lua_settable(L, -3);

	lua_pushstring(L, "ReadKey");
	lua_pushcfunction(L, L_ConsoleReadKey);
	lua_settable(L, -3);

	lua_pushstring(L, "Write");
	lua_pushcfunction(L, L_ConsoleWrite);
	lua_settable(L, -3);

	lua_pushstring(L, "SetColor");
	lua_pushcfunction(L, L_SetTextColor);
	lua_settable(L, -3);

	lua_pushstring(L, "GetColor");
	lua_pushcfunction(L, L_GetTextColor);
	lua_settable(L, -3);

	lua_pushstring(L, "SetVisible");
	lua_pushcfunction(L, L_ToggleConsole);
	lua_settable(L, -3);

	lua_pushstring(L, "SetTitle");
	lua_pushcfunction(L, L_SetTitle);
	lua_settable(L, -3);

	lua_pushstring(L, "Destroy");
	lua_pushcfunction(L, L_ConsoleDestroy);
	lua_settable(L, -3);

	lua_pushstring(L, "Create");
	lua_pushcfunction(L, L_ConsoleCreate);
	lua_settable(L, -3);

	lua_pushstring(L, "Clear");
	lua_pushcfunction(L, L_cls);
	lua_settable(L, -3);

	lua_pushstring(L, "GetInfo");
	lua_pushcfunction(L, L_GetConsoleCoords);
	lua_settable(L, -3);

	lua_pushstring(L, "SetCursorPosition");
	lua_pushcfunction(L, L_SetConsoleCoords);
	lua_settable(L, -3);

	lua_setglobal(L, "Console");

	lua_newtable(L);

	lua_pushstring(L, "Play");
	lua_pushcfunction(L, luasound);
	lua_settable(L, -3);

	lua_pushstring(L, "Beep");
	lua_pushcfunction(L, luabeep);
	lua_settable(L, -3);

	lua_pushstring(L, "SendMCS");
	lua_pushcfunction(L, luasoundcommand);
	lua_settable(L, -3);

	lua_setglobal(L, "Sound");

	lua_getglobal(L, "string");
	lua_pushstring(L, "equal");
	lua_pushcfunction(L, GetStringEqual);
	lua_settable(L, -3);
	lua_pop(L, 1);

	lua_getglobal(L, "table");
	lua_pushstring(L, "first");
	lua_pushcfunction(L, TableFirst);
	lua_settable(L, -3);
	lua_pushstring(L, "select");
	lua_pushcfunction(L, TableSelect);
	lua_settable(L, -3);
	lua_pop(L, 1);

	lua_pushcfunction(L, L_DebugBreak);
	lua_setglobal(L, "Break");

	lua_pushcfunction(L, L_GetGlobalMemoryStatus);
	lua_setglobal(L, "GlobalMemoryStatus");

	lua_pushcfunction(L, L_GetHost);
	lua_setglobal(L, "Dns");

	lua_pushcfunction(L, L_GetComputerName);
	lua_setglobal(L, "GetComputerName");

	lua_pushcfunction(L, GetLastErrorAsMessage);
	lua_setglobal(L, "GetLastError");

	lua_pushcfunction(L, lua_uuid);
	lua_setglobal(L, "UUID");

	lua_pushcfunction(L, lua_sleep);
	lua_setglobal(L, "Sleep");

	lua_pushcfunction(L, Time);
	lua_setglobal(L, "Time");

	lua_pushcfunction(L, CRC32);
	lua_setglobal(L, "CRC32");

	lua_pushcfunction(L, luasetenv);
	lua_setglobal(L, "setenv");

	lua_pushcfunction(L, luagetenv);
	lua_setglobal(L, "getenv");

	lua_newtable(L);

	lua_pushstring(L, "Create");
	lua_pushcfunction(L, NewEnvironment);
	lua_settable(L, -3);

	lua_pushstring(L, "Get");
	lua_pushcfunction(L, GetEnvironment);
	lua_settable(L, -3);

	lua_pushstring(L, "GetOrCreate");
	lua_pushcfunction(L, GetCreateEnvironment);
	lua_settable(L, -3);

	lua_pushstring(L, "Meta");
	lua_pushcfunction(L, GetAllEnvironment);
	lua_settable(L, -3);

	lua_setglobal(L, "Env");

	return 0;
}