#include "LuaFileSystem.h"
#include <Windows.h>
#include <time.h>

static char _PATH[MAX_PATH+2];

const char * lua_topath(lua_State*L, int idx, bool wildcard=false) {
	size_t len;
	const char * fromlua = luaL_checklstring(L, idx, &len);
	const char * filter = wildcard ? luaL_optstring(L, idx+1, "*") : "*";
	char c;
	if (len+strlen(filter) >= MAX_PATH)
		luaL_error(L, "%s is too long to be a path!", fromlua);

	for (int n = 0; n < len; n++) {

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

		c = _PATH[len-1];

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

int GetCurrent(lua_State*L) {
	GetCurrentDirectory(MAX_PATH, _PATH);
	lua_pushstring(L, _PATH);
	return 1;
}

int GetFiles(lua_State*L) {

	const char * path = lua_topath(L, 1, true);

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

int GetDirectories(lua_State*L) {

	const char * path = lua_topath(L, 1, true);

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

time_t FILETIME_to_time_t(const FILETIME *lpFileTime) {

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

bool get_file_information(const char * path, WIN32_FIND_DATA* data)
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

int GetFileInfo(lua_State*L) {

	const char * path = lua_topath(L, 1);
	WIN32_FIND_DATA data;

	if (get_file_information(path, &data)) {

		lua_pop(L, 1);

		lua_newtable(L);

		lua_pushstring(L, "isFolder");
		lua_pushboolean(L, data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
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

int lua_CopyFile(lua_State *L) {

	const char * src = luaL_checkstring(L, 1);
	const char * dst = luaL_checkstring(L, 2);

	BOOL ret = CopyFile(src, dst, !lua_toboolean(L, 3));

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, ret);

	return 1;
}

int lua_MoveFile(lua_State *L) {

	const char * src = luaL_checkstring(L, 1);
	const char * dst = luaL_checkstring(L, 2);

	BOOL ret = MoveFile(src, dst);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, ret);

	return 1;
}

int lua_DeleteFile(lua_State *L) {

	const char * src = luaL_checkstring(L, 1);

	BOOL ret = DeleteFile(src);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, ret);

	return 1;
}

int lua_CreateDirectory(lua_State *L) {

	const char * src = luaL_checkstring(L, 1);

	BOOL ret = CreateDirectory(src, NULL);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, ret);

	return 1;
}

int lua_RemoveDirectory(lua_State *L) {

	const char * src = luaL_checkstring(L, 1);

	BOOL ret = RemoveDirectory(src);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, ret);

	return 1;
}

int lua_Rename(lua_State *L) {

	const char * src = luaL_checkstring(L, 1);
	const char * dst = luaL_checkstring(L, 2);

	BOOL ret = rename(src, dst);

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, ret == 0);

	return 1;
}

int lua_TempFile(lua_State *L) {

	char temp[MAX_PATH];

	GetTempPath(MAX_PATH, temp);

	if (lua_gettop(L) <= 0 || !lua_toboolean(L, 1)) {

		GetTempFileName(temp, "gff", 0, temp);
	}

	lua_pushstring(L, temp);

	return 1;
}

int lua_SetCurrentDirectory(lua_State *L) {

	BOOL ok = SetCurrentDirectory(luaL_checkstring(L, 1));
	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, ok != 0);
	return 1;
}

int lua_GetAllAvailableDrives(lua_State *L) {

	DWORD drives = GetLogicalDrives();
	DWORD mask = 1;
	size_t cnt = 0;

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
	DWORD type;
	char drive[5] = {0};
	strcpy(drive, "A:\\");

	ULARGE_INTEGER lpFreeBytesAvailableToCaller;
	ULARGE_INTEGER lpTotalNumberOfBytes;
	ULARGE_INTEGER lpTotalNumberOfFreeBytes;

	for (int letter = 'A'; letter < 'Z' + 1; letter++) {

		if (drives & mask) {
			cnt++;

			drive[0] = (char)letter;

			type = GetDriveType(drive);

			if (!GetDiskFreeSpaceExA(drive, &lpFreeBytesAvailableToCaller, &lpTotalNumberOfBytes, &lpTotalNumberOfFreeBytes)) {
				memset(&lpFreeBytesAvailableToCaller, 0, sizeof(ULARGE_INTEGER));
				memset(&lpTotalNumberOfBytes, 0, sizeof(ULARGE_INTEGER));
				memset(&lpTotalNumberOfFreeBytes, 0, sizeof(ULARGE_INTEGER));
			}

			lua_createtable(L, 0, 6);

			lua_pushstring(L, "Drive");
			lua_pushfstring(L, "%c", letter);
			lua_settable(L, -3);

			lua_pushstring(L, "Mask");
			lua_pushinteger(L, mask);
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

			lua_rawseti(L, -2, ++n);
		}

		mask *= 2;
	}

	return 1;
}