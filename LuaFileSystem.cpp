#include "LuaFileSystem.h"
#include <Windows.h>
#include <time.h>
#include <io.h>
#include "luawchar.h"

#define tolstream(L)	((LStream *)luaL_checkudata(L, 1, LUA_FILEHANDLE))
#define MAX_PATH_LENGTH 1024

static char _PATH[MAX_PATH_LENGTH];
static wchar_t _PATHW[MAX_PATH_LENGTH];

const wchar_t* lua_topathw(lua_State* L, int idx, bool wildcard = false) {

	LuaWChar* fromlua = lua_towchar(L, idx);
	wchar_t* filter = L"*";

	if (wildcard && lua_type(L, idx + 1) == LUA_TUSERDATA) {
		filter = lua_towchar(L, idx + 1)->str;
	}

	wchar_t c;
	if (fromlua->len + wcslen(filter) >= MAX_PATH_LENGTH)
		luaL_error(L, "%s is too long to be a path!", fromlua);

	for (size_t n = 0; n < fromlua->len; n++) {

		c = fromlua->str[n];

		if (c == L'/') {
			_PATHW[n] = L'\\';
		}
		else {
			_PATHW[n] = fromlua->str[n];
		}
	}

	_PATHW[fromlua->len] = L'\0';

	if (wildcard) {

		c = _PATHW[fromlua->len - 1];

		if (c == L'/' || c == L'\\') {

			wcscat(_PATHW, filter);
		}
		else {
			wcscat(_PATHW, L"\\");
			wcscat(_PATHW, filter);
		}
	}

	return _PATHW;
}

const char* lua_topath(lua_State* L, int idx, bool wildcard = false) {
	size_t len;
	const char* fromlua = luaL_checklstring(L, idx, &len);
	const char* filter = wildcard ? luaL_optstring(L, idx + 1, "*") : "*";
	char c;
	if (len + strlen(filter) >= MAX_PATH_LENGTH)
		luaL_error(L, "%s is too long to be a path!", fromlua);

	for (size_t n = 0; n < len; n++) {

		c = fromlua[n];

		if (c == '/') {
			_PATH[n] = '\\';
		}
		else {
			_PATH[n] = fromlua[n];
		}
	}

	_PATH[len] = '\0';

	if (wildcard) {

		c = _PATH[len - 1];

		if (c == '/' || c == '\\') {

			strcat(_PATH, filter);
		}
		else {
			strcat(_PATH, "\\");
			strcat(_PATH, filter);
		}
	}

	return _PATH;
}

int GetCurrentWide(lua_State* L) {
	GetCurrentDirectoryW(MAX_PATH_LENGTH, _PATHW);
	lua_pushwchar(L, _PATHW);
	return 1;
}

int GetCurrent(lua_State* L) {
	GetCurrentDirectory(MAX_PATH_LENGTH, _PATH);
	lua_pushstring(L, _PATH);
	return 1;
}

int GetFiles(lua_State* L) {

	const char* path = lua_topath(L, 1, true);

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	lua_pop(L, 1);
	lua_newtable(L);

	hFind = FindFirstFile(path, &FindFileData);
	int n = 0;
	if (hFind != INVALID_HANDLE_VALUE) {
		do {

			if (strcmp(FindFileData.cFileName, ".") != 0 &&
				strcmp(FindFileData.cFileName, "..") != 0 &&
				!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				lua_pushstring(L, FindFileData.cFileName);
				lua_rawseti(L, -2, ++n);
			}
		} while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}

	return 1;
}

int GetDirectories(lua_State* L) {

	const char* path = lua_topath(L, 1, true);

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	lua_pop(L, 1);
	lua_newtable(L);

	hFind = FindFirstFile(path, &FindFileData);
	int n = 0;
	if (hFind != INVALID_HANDLE_VALUE) {
		do {

			if (strcmp(FindFileData.cFileName, ".") != 0 &&
				strcmp(FindFileData.cFileName, "..") != 0 &&
				FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				lua_pushstring(L, FindFileData.cFileName);
				lua_rawseti(L, -2, ++n);
			}
		} while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}

	return 1;
}

time_t FILETIME_to_time_t(const FILETIME* lpFileTime) {

	time_t result;

	SYSTEMTIME st;

	struct tm tmp;

	FileTimeToSystemTime(lpFileTime, &st);

	memset(&tmp, 0, sizeof(struct tm));

	tmp.tm_mday = st.wDay;
	tmp.tm_mon = st.wMonth - 1;
	tmp.tm_year = st.wYear - 1900;

	tmp.tm_sec = st.wSecond;
	tmp.tm_min = st.wMinute;
	tmp.tm_hour = st.wHour;

	result = mktime(&tmp);

	return result;
}

bool get_file_informationw(const wchar_t* path, WIN32_FIND_DATAW* data)
{
	HANDLE h = FindFirstFileW(path, data);
	if (h == INVALID_HANDLE_VALUE) {
		return false;
	}
	else {
		FindClose(h);
		return true;
	}
}

bool get_file_information(const char* path, WIN32_FIND_DATA* data)
{
	HANDLE h = FindFirstFile(path, data);
	if (h == INVALID_HANDLE_VALUE) {
		return false;
	}
	else {
		FindClose(h);
		return true;
	}
}

typedef luaL_Stream LStream;

static LStream* newprefile(lua_State* L) {
	LStream* p = (LStream*)lua_newuserdata(L, sizeof(LStream));
	p->closef = NULL;  /* mark file handle as 'closed' */
	luaL_setmetatable(L, LUA_FILEHANDLE);
	return p;
}

static int io_fclose(lua_State* L) {
	LStream* p = tolstream(L);
	int res = fclose(p->f);
	return luaL_fileresult(L, (res == 0), NULL);
}

static LStream* newfile(lua_State* L) {
	LStream* p = newprefile(L);
	p->f = NULL;
	p->closef = &io_fclose;
	return p;
}

int	RenameWide(lua_State* L) {

	size_t len;
	LuaWChar* src = lua_towchar(L, 1);
	LuaWChar* dst = lua_towchar(L, 1);

	lua_pushboolean(L, _wrename(src->str, dst->str) == 0);

	return 1;
}

int lua_SetFileAttributes(lua_State* L) {

	LuaWChar* wsrc = (LuaWChar*)luaL_testudata(L, 1, LUAWCHAR);
	DWORD mask = (DWORD)luaL_checkinteger(L, 2);

	if (wsrc) {
		BOOL ret = SetFileAttributesW(wsrc->str, mask);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, ret);
	}
	else {
		const char* src = luaL_checkstring(L, 1);

		BOOL ret = SetFileAttributes(src, mask);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, ret);
	}

	return 1;
}

int OpenFileWide(lua_State* L) {

	LuaWChar* filename = lua_towchar(L, 1);
	LuaWChar* mode = lua_towchar(L, 2);

	LStream* p = newfile(L);
	p->f = _wfopen(filename->str, mode->str);

	if (p->f == NULL) {
		lua_pushnil(L);
	}

	return 1;
}

int GetAllInFolderWide(lua_State* L) {

	const wchar_t* path = lua_topathw(L, 1, true);

	WIN32_FIND_DATAW FindFileData;
	HANDLE hFind;

	lua_pop(L, 1);
	lua_newtable(L);

	hFind = FindFirstFileW(path, &FindFileData);
	int n = 0;
	if (hFind != INVALID_HANDLE_VALUE) {
		do {

			if (wcscmp(FindFileData.cFileName, L".") != 0 &&
				wcscmp(FindFileData.cFileName, L"..") != 0)
			{
				lua_createtable(L, 0, 8);

				lua_pushstring(L, "FileName");
				lua_pushwchar(L, FindFileData.cFileName);
				lua_settable(L, -3);

				lua_pushstring(L, "AlternateFileName");
				lua_pushwchar(L, FindFileData.cAlternateFileName);
				lua_settable(L, -3);

				lua_pushstring(L, "isFolder");
				lua_pushboolean(L, (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
				lua_settable(L, -3);

				lua_pushstring(L, "Attributes");
				lua_pushinteger(L, FindFileData.dwFileAttributes);
				lua_settable(L, -3);

				lua_pushstring(L, "Size");
				lua_pushinteger(L, FindFileData.nFileSizeLow);
				lua_settable(L, -3);

				lua_pushstring(L, "Creation");
				lua_pushinteger(L, FILETIME_to_time_t(&FindFileData.ftCreationTime));
				lua_settable(L, -3);

				lua_pushstring(L, "Access");
				lua_pushinteger(L, FILETIME_to_time_t(&FindFileData.ftLastAccessTime));
				lua_settable(L, -3);

				lua_pushstring(L, "Write");
				lua_pushinteger(L, FILETIME_to_time_t(&FindFileData.ftLastWriteTime));
				lua_settable(L, -3);

				lua_rawseti(L, -2, ++n);
			}
		} while (FindNextFileW(hFind, &FindFileData));
		FindClose(hFind);
	}

	return 1;
}


int GetAllInFolder(lua_State* L) {

	const char* path = lua_topath(L, 1, true);

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	lua_pop(L, 1);
	lua_newtable(L);

	hFind = FindFirstFile(path, &FindFileData);
	int n = 0;
	if (hFind != INVALID_HANDLE_VALUE) {
		do {

			if (strcmp(FindFileData.cFileName, ".") != 0 &&
				strcmp(FindFileData.cFileName, "..") != 0)
			{
				lua_createtable(L, 0, 8);

				lua_pushstring(L, "FileName");
				lua_pushstring(L, FindFileData.cFileName);
				lua_settable(L, -3);

				lua_pushstring(L, "AlternateFileName");
				lua_pushstring(L, FindFileData.cAlternateFileName);
				lua_settable(L, -3);

				lua_pushstring(L, "isFolder");
				lua_pushboolean(L, (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
				lua_settable(L, -3);

				lua_pushstring(L, "Attributes");
				lua_pushinteger(L, FindFileData.dwFileAttributes);
				lua_settable(L, -3);

				lua_pushstring(L, "Size");
				lua_pushinteger(L, FindFileData.nFileSizeLow);
				lua_settable(L, -3);

				lua_pushstring(L, "Creation");
				lua_pushinteger(L, FILETIME_to_time_t(&FindFileData.ftCreationTime));
				lua_settable(L, -3);

				lua_pushstring(L, "Access");
				lua_pushinteger(L, FILETIME_to_time_t(&FindFileData.ftLastAccessTime));
				lua_settable(L, -3);

				lua_pushstring(L, "Write");
				lua_pushinteger(L, FILETIME_to_time_t(&FindFileData.ftLastWriteTime));
				lua_settable(L, -3);

				lua_rawseti(L, -2, ++n);
			}
		} while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}

	return 1;
}


int GetFileInfoWide(lua_State* L) {

	const wchar_t* path = lua_topathw(L, 1);
	WIN32_FIND_DATAW data;

	if (get_file_informationw(path, &data)) {

		lua_pop(L, 1);

		lua_createtable(L, 0, 8);

		lua_pushstring(L, "FileName");
		lua_pushwchar(L, data.cFileName);
		lua_settable(L, -3);

		lua_pushstring(L, "AlternateFileName");
		lua_pushwchar(L, data.cAlternateFileName);
		lua_settable(L, -3);

		lua_pushstring(L, "isFolder");
		lua_pushboolean(L, (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
		lua_settable(L, -3);

		lua_pushstring(L, "Attributes");
		lua_pushinteger(L, data.dwFileAttributes);
		lua_settable(L, -3);

		lua_pushstring(L, "Size");
		lua_pushinteger(L, data.nFileSizeLow);
		lua_settable(L, -3);

		lua_pushstring(L, "Creation");
		lua_pushinteger(L, FILETIME_to_time_t(&data.ftCreationTime));
		lua_settable(L, -3);

		lua_pushstring(L, "Access");
		lua_pushinteger(L, FILETIME_to_time_t(&data.ftLastAccessTime));
		lua_settable(L, -3);

		lua_pushstring(L, "Write");
		lua_pushinteger(L, FILETIME_to_time_t(&data.ftLastWriteTime));
		lua_settable(L, -3);
	}
	else {
		lua_pop(L, 1);
		lua_pushnil(L);
	}

	return 1;
}

int GetFileInfo(lua_State* L) {

	const char* path = lua_topath(L, 1);
	WIN32_FIND_DATA data;

	if (get_file_information(path, &data)) {

		lua_pop(L, 1);

		lua_createtable(L, 0, 8);

		lua_pushstring(L, "FileName");
		lua_pushstring(L, data.cFileName);
		lua_settable(L, -3);

		lua_pushstring(L, "AlternateFileName");
		lua_pushstring(L, data.cAlternateFileName);
		lua_settable(L, -3);

		lua_pushstring(L, "isFolder");
		lua_pushboolean(L, (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
		lua_settable(L, -3);

		lua_pushstring(L, "Attributes");
		lua_pushinteger(L, data.dwFileAttributes);
		lua_settable(L, -3);

		lua_pushstring(L, "Size");
		lua_pushinteger(L, data.nFileSizeLow);
		lua_settable(L, -3);

		lua_pushstring(L, "Creation");
		lua_pushinteger(L, FILETIME_to_time_t(&data.ftCreationTime));
		lua_settable(L, -3);

		lua_pushstring(L, "Access");
		lua_pushinteger(L, FILETIME_to_time_t(&data.ftLastAccessTime));
		lua_settable(L, -3);

		lua_pushstring(L, "Write");
		lua_pushinteger(L, FILETIME_to_time_t(&data.ftLastWriteTime));
		lua_settable(L, -3);
	}
	else {
		lua_pop(L, 1);
		lua_pushnil(L);
	}

	return 1;
}

int lua_CopyFile(lua_State* L) {

	const char* src = luaL_checkstring(L, 1);
	const char* dst = luaL_checkstring(L, 2);

	BOOL ret = CopyFile(src, dst, !lua_toboolean(L, 3));

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, ret);

	return 1;
}

int lua_MoveFile(lua_State* L) {

	const char* src = luaL_checkstring(L, 1);
	const char* dst = luaL_checkstring(L, 2);

	BOOL ret = MoveFile(src, dst);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, ret);

	return 1;
}

int lua_DeleteFile(lua_State* L) {

	LuaWChar* wsrc = (LuaWChar*)luaL_testudata(L, 1, LUAWCHAR);

	if (wsrc) {
		BOOL ret = DeleteFileW(wsrc->str);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, ret);
	}
	else {
		const char* src = luaL_checkstring(L, 1);

		BOOL ret = DeleteFile(src);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, ret);
	}

	return 1;
}

int lua_CreateDirectory(lua_State* L) {

	LuaWChar* wsrc = (LuaWChar*)luaL_testudata(L, 1, LUAWCHAR);

	if (wsrc) {
		BOOL ret = CreateDirectoryW(wsrc->str, NULL);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, ret);
	}
	else {
		const char* src = luaL_checkstring(L, 1);

		BOOL ret = CreateDirectory(src, NULL);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, ret);
	}

	return 1;
}

int lua_RemoveDirectory(lua_State* L) {

	LuaWChar* wsrc = (LuaWChar*)luaL_testudata(L, 1, LUAWCHAR);

	if (wsrc) {
		BOOL ret = RemoveDirectoryW(wsrc->str);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, ret);
	}
	else {
		const char* src = luaL_checkstring(L, 1);

		BOOL ret = RemoveDirectory(src);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, ret);
	}

	return 1;
}

int lua_Rename(lua_State* L) {

	const char* src = luaL_checkstring(L, 1);
	const char* dst = luaL_checkstring(L, 2);

	BOOL ret = rename(src, dst);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, ret == 0);

	return 1;
}

int lua_TempFile(lua_State* L) {

	char temp[MAX_PATH_LENGTH];

	GetTempPath(MAX_PATH_LENGTH, temp);

	if (lua_gettop(L) <= 0 || !lua_toboolean(L, 1)) {

		GetTempFileName(temp, "gff", 0, temp);
	}

	lua_pushstring(L, temp);

	return 1;
}

int lua_SetCurrentDirectory(lua_State* L) {

	LuaWChar* wsrc = (LuaWChar*)luaL_testudata(L, 1, LUAWCHAR);

	if (wsrc) {
		BOOL ret = SetCurrentDirectoryW(wsrc->str);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, ret);
	}
	else {
		const char* src = luaL_checkstring(L, 1);

		BOOL ret = SetCurrentDirectory(src);

		lua_pop(L, lua_gettop(L));
		lua_pushboolean(L, ret);
	}

	return 1;
}

void PushDrive(lua_State* L, const char* drive) {

	ULARGE_INTEGER lpFreeBytesAvailableToCaller;
	ULARGE_INTEGER lpTotalNumberOfBytes;
	ULARGE_INTEGER lpTotalNumberOfFreeBytes;

	DWORD type = GetDriveType(drive);

	if (!GetDiskFreeSpaceExA(drive, &lpFreeBytesAvailableToCaller, &lpTotalNumberOfBytes, &lpTotalNumberOfFreeBytes)) {
		memset(&lpFreeBytesAvailableToCaller, 0, sizeof(ULARGE_INTEGER));
		memset(&lpTotalNumberOfBytes, 0, sizeof(ULARGE_INTEGER));
		memset(&lpTotalNumberOfFreeBytes, 0, sizeof(ULARGE_INTEGER));
	}

	lua_createtable(L, 0, 6);

	lua_pushstring(L, "Drive");
	lua_pushfstring(L, "%c", drive[0]);
	lua_settable(L, -3);

	lua_pushstring(L, "Type");
	lua_pushinteger(L, type);
	lua_settable(L, -3);

	lua_pushstring(L, "FreeBytesAvailableToCaller");
	lua_pushinteger(L, lpFreeBytesAvailableToCaller.QuadPart);
	lua_settable(L, -3);

	lua_pushstring(L, "TotalNumberOfBytes");
	lua_pushinteger(L, lpTotalNumberOfBytes.QuadPart);
	lua_settable(L, -3);

	lua_pushstring(L, "TotalNumberOfFreeBytes");
	lua_pushinteger(L, lpTotalNumberOfFreeBytes.QuadPart);
	lua_settable(L, -3);
}

int lua_GetAllAvailableDrives(lua_State* L) {

	DWORD drives = GetLogicalDrives();
	DWORD mask = 1;
	size_t cnt = 0;
	size_t len;

	const char* opt = luaL_optlstring(L, 1, NULL, &len);
	char letter = 0;

	if (opt && len == 1) {
		letter = (char)toupper(opt[0]);
		if (letter < 'A' || letter > 'Z') {
			letter = 0;
		}
	}

	char drive[5] = { 0 };
	strcpy(drive, "A:\\");

	if (opt != NULL) {

		lua_pop(L, lua_gettop(L));

		if (letter != 0) {
			drive[0] = letter;
			PushDrive(L, drive);
		}
		else {
			lua_pushnil(L);
		}

		return 1;
	}

	for (int letter = 'A'; letter < 'Z' + 1; letter++) {

		if (drives & mask) {
			cnt++;
		}

		mask *= 2;
	}

	lua_pop(L, lua_gettop(L));
	lua_createtable(L, cnt, 0);

	mask = 1;

	int n = 0;

	for (int letter = 'A'; letter < 'Z' + 1; letter++) {

		if (drives & mask) {
			cnt++;

			drive[0] = (char)letter;

			PushDrive(L, drive);

			lua_rawseti(L, -2, ++n);
		}

		mask *= 2;
	}

	return 1;
}