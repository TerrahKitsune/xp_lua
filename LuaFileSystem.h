#pragma once
#include "lua_main_incl.h"

int GetFiles(lua_State*L);
int GetDirectories(lua_State*L);
int GetFileInfo(lua_State*L);
int lua_CopyFile(lua_State *L);
int lua_MoveFile(lua_State *L);
int lua_DeleteFile(lua_State *L);
int lua_CreateDirectory(lua_State *L);
int lua_RemoveDirectory(lua_State *L);
int lua_Rename(lua_State *L);
int GetCurrent(lua_State*L);
int lua_TempFile(lua_State *L);
int lua_SetCurrentDirectory(lua_State *L);