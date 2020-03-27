#pragma once
#include "lua_main_incl.h"
static const char* LUAFILEASYNC = "LUAFILEASYNC";

#include <windows.h>
#include <stdio.h>
#include <process.h>

#define FA_CMD_NONE 0
#define FA_CMD_REWIND 1
#define FA_CMD_SEEK_SET 2
#define FA_CMD_SEEK_CUR 3
#define FA_CMD_SEEK_END 4
#define FA_CMD_TELL 5
#define FA_CMD_EOF 6
#define FA_CMD_READ 7

typedef struct LuaFileAsyncThreadInfo {

	volatile bool alive;
	FILE* file;
	HANDLE hThread;
	unsigned int threadID;
	CRITICAL_SECTION CriticalSection;
	BYTE* buffer;

	size_t readwritebuffersize;
	size_t bytestoread;

	size_t buffersize;
	size_t currentlen;
	volatile DWORD command;

	volatile bool stop;
	volatile long seek;

} LuaFileAsyncThreadInfo;

typedef struct LuaFileAsync {

	LuaFileAsyncThreadInfo* thread;

} LuaFileAsync;


LuaFileAsync* lua_pushluafileasync(lua_State* L);
LuaFileAsync* lua_toluafileasync(lua_State* L, int index);

int OpenFileAsync(lua_State* L);
int LuaIsBusy(lua_State* L);
int LuaTell(lua_State* L);
int LuaSeek(lua_State* L);
int LuaRewind(lua_State* L);
int LuaIsEof(lua_State* L);
int LuaRead(lua_State* L);
int LuaGetReadWriteBufferStatus(lua_State* L);
int LuaEmptyBuffer(lua_State* L);

int luafileasync_gc(lua_State* L);
int luafileasync_tostring(lua_State* L);