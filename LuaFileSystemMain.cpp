#include "LuaFileSystemMain.h"
#include "LuaFileSystem.h"

static const struct luaL_Reg filesystemfuncs[] = {
	{ "GetAll", GetAllInFolder },
	{ "GetFiles", GetFiles },
	{ "GetDirectories", GetDirectories },
	{ "GetFileInfo", GetFileInfo },
	{ "Copy", lua_CopyFile },
	{ "Move", lua_MoveFile },
	{ "Delete", lua_DeleteFile },
	{ "CreateDirectory", lua_CreateDirectory },
	{ "RemoveDirectory", lua_RemoveDirectory },
	{ "Rename", lua_Rename },
	{ "CurrentDirectory", GetCurrent },
	{ "GetTempFileName", lua_TempFile },
	{ "SetCurrentDirectory", lua_SetCurrentDirectory },
	{ "GetDrives", lua_GetAllAvailableDrives },
	{ "SetAttributes", lua_SetFileAttributes},
	{ "GetAllWide",  GetAllInFolderWide },
	{ "OpenFileWide", OpenFileWide },
	{ "RenameWide", RenameWide },
	{ "GetFileInfoWide", GetFileInfoWide },
	{ "CurrentDirectoryWide", GetCurrentWide },
	{ NULL, NULL }
};

int luaopen_filesystem(lua_State* L) {

	luaL_newlibtable(L, filesystemfuncs);
	luaL_setfuncs(L, filesystemfuncs, 0);

	return 1;
}