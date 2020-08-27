#include "keybif.h"
#include <string.h>
#include "List.h"

int CreateKeyBif(lua_State* L) {

	const char* fileName = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);

	List * folders = list_CreateList();
	List * files;
	FileEntry * folderEntry;
	FileEntry* fileEntry;
	WIN32_FIND_DATA data;

	if (!folders) {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Out of memory");
		return 2;
	}

	DumpStack(L);

	char error[MAX_PATH*2];
	memset(error, 0, MAX_PATH * 2);

	size_t len;
	const char* str;

	int n = 0;
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		DumpStack(L);

		str = lua_tolstring(L, -1, &len);
		HANDLE h = FindFirstFile(str, &data);
		if (h != INVALID_HANDLE_VALUE && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {

			FindClose(h);

			folderEntry = (FileEntry*)gff_calloc(1, sizeof(FileEntry));

			if (!folderEntry) {
				_snprintf(error, MAX_PATH * 2, "Out of memory");
				goto CLEAN;
			}

			folderEntry->Key = (char*)gff_calloc(len+1, sizeof(char));

			if (!folderEntry->Key) {
				gff_free(folderEntry);
				_snprintf(error, MAX_PATH * 2, "Out of memory");
				goto CLEAN;
			}
			else {
				memcpy(folderEntry->Key, str, len);
			}

			folderEntry->Data = list_CreateList();

			if (!folderEntry->Data) {
				gff_free(folderEntry->Key);
				gff_free(folderEntry);
				_snprintf(error, MAX_PATH * 2, "Out of memory");
				goto CLEAN;
			}

			if (list_Add(folders, folderEntry) < 0) {
				gff_free(folderEntry->Key);
				gff_free(folderEntry->Data);
				gff_free(folderEntry);
				_snprintf(error, MAX_PATH * 2, "Out of memory");
				goto CLEAN;
			}
		}
		else {

			if (h != INVALID_HANDLE_VALUE) {
				FindClose(h);
			}

			_snprintf(error, MAX_PATH * 2, "Unable to open folder %s", lua_tostring(L, -1));
			goto CLEAN;
		}

		n++;
		lua_pop(L, 1);
	}

	DumpStack(L);

	CLEAN:

	for (size_t i = 0; i < folders->len; i++)
	{
		folderEntry = (FileEntry*)folders->data[i];
		files = (List*)folderEntry->Data;
		folders->data[i] = NULL;

		for (size_t j = 0; j < files->len; j++)
		{
			fileEntry = (FileEntry*)files->data[j];
			files->data[j] = NULL;

			gff_free(fileEntry->Data);
			gff_free(fileEntry->Key);
			gff_free(fileEntry);
		}

		list_Destroy(files);
		gff_free(folderEntry->Key);
		gff_free(folderEntry);		
	}

	list_Destroy(folders);

	if (error[0] != '\0') {
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, error);
		return 2;
	}

	return 1;
}

LuaKeyBif* lua_pushkeybif(lua_State* L) {
	LuaKeyBif* keybif = (LuaKeyBif*)lua_newuserdata(L, sizeof(LuaKeyBif));
	if (keybif == NULL)
		luaL_error(L, "Unable to push zip");
	luaL_getmetatable(L, KEYBIF);
	lua_setmetatable(L, -2);
	memset(keybif, 0, sizeof(LuaKeyBif));
	return keybif;
}

LuaKeyBif* lua_tokeybif(lua_State* L, int index) {
	LuaKeyBif* keybif = (LuaKeyBif*)luaL_checkudata(L, index, KEYBIF);
	if (keybif == NULL)
		luaL_error(L, "parameter is not a %s", KEYBIF);
	return keybif;
}

int keybif_gc(lua_State* L) {

	LuaKeyBif* keybif = lua_tokeybif(L, 1);

	return 0;
}

int keybif_tostring(lua_State* L) {
	char tim[100];
	sprintf(tim, "KeyBif: 0x%08X", lua_tokeybif(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}