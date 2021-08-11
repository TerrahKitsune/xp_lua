#include "FileAsync.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> 

void ReadAsyncFile(LuaFileAsyncThreadInfo* threadInfo, size_t bytestoread, size_t buffersize) {

	BYTE* buffer = (BYTE*)gff_malloc(buffersize);
	size_t read;
	size_t total = 0;

	if (!buffer) {
		return;
	}

	while (threadInfo->alive && !threadInfo->stop) {

		read = fread(buffer, sizeof(BYTE), buffersize, threadInfo->file);

		if (read > 0) {

			EnterCriticalSection(&threadInfo->CriticalSection);

			for (size_t i = 0; i < read; i++)
			{
				while (threadInfo->currentlen >= threadInfo->buffersize) {

					LeaveCriticalSection(&threadInfo->CriticalSection);

					Sleep(1);

					if (!threadInfo->alive || threadInfo->stop) {
						gff_free(buffer);
						return;
					}

					EnterCriticalSection(&threadInfo->CriticalSection);
				}

				threadInfo->buffer[threadInfo->currentlen++] = buffer[i];

				if (++total >= bytestoread && bytestoread > 0) {

					LeaveCriticalSection(&threadInfo->CriticalSection);
					gff_free(buffer);
					return;
				}
			}

			LeaveCriticalSection(&threadInfo->CriticalSection);
		}

		if (read != buffersize && feof(threadInfo->file)) {

			gff_free(buffer);
			return;
		}
	}

	gff_free(buffer);
}

unsigned __stdcall FileAsyncProcessThreadFunction(void* pArguments) {

	LuaFileAsyncThreadInfo* threadInfo = (LuaFileAsyncThreadInfo*)pArguments;

	while (threadInfo->alive) {

		if (threadInfo->command == FA_CMD_NONE) {
			Sleep(1);
		}
		else {

			if (threadInfo->command == FA_CMD_REWIND) {
				rewind(threadInfo->file);
			}
			else if (threadInfo->command == FA_CMD_SEEK_SET) {
				threadInfo->seek = fseek(threadInfo->file, threadInfo->seek, SEEK_SET);
			}
			else if (threadInfo->command == FA_CMD_SEEK_CUR) {
				threadInfo->seek = fseek(threadInfo->file, threadInfo->seek, SEEK_CUR);
			}
			else if (threadInfo->command == FA_CMD_SEEK_END) {
				threadInfo->seek = fseek(threadInfo->file, threadInfo->seek, SEEK_END);
			}
			else if (threadInfo->command == FA_CMD_TELL) {
				threadInfo->seek = ftell(threadInfo->file);
			}
			else if (threadInfo->command == FA_CMD_EOF) {
				threadInfo->seek = feof(threadInfo->file);
			}
			else if (threadInfo->command == FA_CMD_READ) {
				ReadAsyncFile(threadInfo, threadInfo->bytestoread, threadInfo->readwritebuffersize);
			}

			threadInfo->command = FA_CMD_NONE;
		}
	}

	fclose(threadInfo->file);

	return 0;
}

void SetCommand(LuaFileAsyncThreadInfo* info, DWORD command) {

	info->stop = false;
	info->command = command;
}

bool WaitTillFinished(LuaFileAsyncThreadInfo* info, lua_State* L) {

	while (info->command != FA_CMD_NONE) {

		Sleep(1);

		if (info->command == FA_CMD_READ) {

			EnterCriticalSection(&info->CriticalSection);

			if(info->buffersize >= info->currentlen){
				LeaveCriticalSection(&info->CriticalSection);

				if (L) {
					luaL_error(L, "Buffer is full!");
				}

				return false;
			}
			else {
				LeaveCriticalSection(&info->CriticalSection);
			}
		}
	}

	return true;
}

int OpenFileAsync(lua_State* L) {

	const char* filename = luaL_checkstring(L, 1);
	const char* mode = luaL_checkstring(L, 2);
	size_t buffersize = (size_t)luaL_optinteger(L, 3, 1048576);

	if (buffersize < 1000) {
		buffersize = 1000;
	}

	FILE* file = fopen(filename, mode);

	if (!file) {

		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushfstring(L, "Unable to open file %s with mode %s", filename, mode);

		return 2;
	}
	else {
		lua_pop(L, lua_gettop(L));
	}

	LuaFileAsync* afile = lua_pushluafileasync(L);

	afile->thread = (LuaFileAsyncThreadInfo*)gff_calloc(1, sizeof(LuaFileAsyncThreadInfo));

	if (!afile->thread) {

		fclose(file);
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Not enough memory to allocate buffer");

		return 2;
	}

	afile->thread->file = file;
	afile->thread->alive = true;
	afile->thread->buffer = (BYTE*)gff_malloc(buffersize);
	afile->thread->currentlen = 0;
	afile->thread->buffersize = buffersize;

	if (!afile->thread->buffer) {

		fclose(file);
		gff_free(afile->thread);
		afile->thread = NULL;
		lua_pop(L, lua_gettop(L));
		lua_pushnil(L);
		lua_pushstring(L, "Not enough memory to allocate buffer");

		return 2;
	}

	InitializeCriticalSectionAndSpinCount(&afile->thread->CriticalSection, 0x00000400);

	afile->thread->hThread = (HANDLE)_beginthreadex(NULL, 0, &FileAsyncProcessThreadFunction, afile->thread, 0, &afile->thread->threadID);

	return 1;
}

int LuaSeek(lua_State* L) {

	LuaFileAsync* file = lua_toluafileasync(L, 1);
	long seek = (long)luaL_checkinteger(L, 2);
	int type = (int)luaL_optinteger(L, 3, 0);

	if (!file->thread) {
		luaL_error(L, "FileAsync object is closed");
		return 0;
	}

	WaitTillFinished(file->thread, L);
	file->thread->seek = seek;
	switch (type) {

	case 1:
		SetCommand(file->thread, FA_CMD_SEEK_CUR);
		break;

	case 2:
		SetCommand(file->thread, FA_CMD_SEEK_END);
		break;

	default:
		SetCommand(file->thread, FA_CMD_SEEK_SET);
		break;
	}

	WaitTillFinished(file->thread, L);

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, seek);

	return 1;
}

int LuaTell(lua_State* L) {

	LuaFileAsync* file = lua_toluafileasync(L, 1);

	if (!file->thread) {
		luaL_error(L, "FileAsync object is closed");
		return 0;
	}

	WaitTillFinished(file->thread, L);
	SetCommand(file->thread, FA_CMD_TELL);
	WaitTillFinished(file->thread, L);

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, file->thread->seek);

	return 1;
}

int LuaRewind(lua_State* L) {

	LuaFileAsync* file = lua_toluafileasync(L, 1);

	if (!file->thread) {
		luaL_error(L, "FileAsync object is closed");
		return 0;
	}

	WaitTillFinished(file->thread, L);
	SetCommand(file->thread, FA_CMD_REWIND);
	lua_pop(L, lua_gettop(L));

	return 0;
}

int LuaIsBusy(lua_State* L) {

	LuaFileAsync* file = lua_toluafileasync(L, 1);
	bool isStopping = lua_toboolean(L, 3) != 0;

	if (isStopping) {
		file->thread->stop = true;
	}

	if (lua_toboolean(L, 2)) {
		WaitTillFinished(file->thread, isStopping ? NULL : L);
	}

	lua_pop(L, lua_gettop(L));
	lua_pushboolean(L, file->thread->command != FA_CMD_NONE);

	return 1;
}

int LuaIsEof(lua_State* L) {

	LuaFileAsync* file = lua_toluafileasync(L, 1);

	if (!file->thread) {
		luaL_error(L, "FileAsync object is closed");
		return 0;
	}

	WaitTillFinished(file->thread, L);
	SetCommand(file->thread, FA_CMD_EOF);
	WaitTillFinished(file->thread, L);

	lua_pop(L, lua_gettop(L));
	lua_pushinteger(L, file->thread->seek);

	return 1;
}

int LuaRead(lua_State* L) {

	LuaFileAsync* file = lua_toluafileasync(L, 1);
	size_t bytetoread = (size_t)luaL_optinteger(L, 2, 0);
	size_t readwritebuffersize = (size_t)luaL_optinteger(L, 3, 1024);

	if (readwritebuffersize < 1000) {
		readwritebuffersize = 1000;
	}

	if (!file->thread) {
		luaL_error(L, "FileAsync object is closed");
		return 0;
	}

	WaitTillFinished(file->thread, L);

	file->thread->readwritebuffersize = readwritebuffersize;
	file->thread->bytestoread = bytetoread;

	SetCommand(file->thread, FA_CMD_READ);

	lua_pop(L, lua_gettop(L));

	return 0;
}

int LuaGetReadWriteBufferStatus(lua_State* L) {

	LuaFileAsync* file = lua_toluafileasync(L, 1);

	if (!file->thread) {
		luaL_error(L, "FileAsync object is closed");
		return 0;
	}

	lua_pop(L, lua_gettop(L));
	EnterCriticalSection(&file->thread->CriticalSection);
	lua_pushinteger(L, file->thread->currentlen);
	lua_pushinteger(L, file->thread->buffersize);
	LeaveCriticalSection(&file->thread->CriticalSection);

	return 2;
}

int LuaEmptyBuffer(lua_State* L) {

	LuaFileAsync* file = lua_toluafileasync(L, 1);

	if (!file->thread) {
		luaL_error(L, "FileAsync object is closed");
		return 0;
	}

	lua_pop(L, lua_gettop(L));
	EnterCriticalSection(&file->thread->CriticalSection);
	lua_pushlstring(L, (const char *)file->thread->buffer, file->thread->currentlen);
	lua_pushboolean(L, file->thread->command != FA_CMD_NONE || file->thread->currentlen > 0);
	file->thread->currentlen = 0;
	LeaveCriticalSection(&file->thread->CriticalSection);

	return 2;

}

LuaFileAsync* lua_pushluafileasync(lua_State* L) {

	LuaFileAsync* fileasync = (LuaFileAsync*)lua_newuserdata(L, sizeof(LuaFileAsync));

	if (fileasync == NULL)
		luaL_error(L, "Unable to push fileasync");

	luaL_getmetatable(L, LUAFILEASYNC);
	lua_setmetatable(L, -2);

	memset(fileasync, 0, sizeof(LuaFileAsync));

	return fileasync;
}

LuaFileAsync* lua_toluafileasync(lua_State* L, int index) {

	LuaFileAsync* fileasync = (LuaFileAsync*)luaL_checkudata(L, index, LUAFILEASYNC);

	if (fileasync == NULL)
		luaL_error(L, "parameter is not a %s", LUAFILEASYNC);

	return fileasync;
}

int luafileasync_gc(lua_State* L) {

	LuaFileAsync* fileasync = lua_toluafileasync(L, 1);
	DWORD waitResult = 0;

	if (fileasync->thread) {

		while (fileasync->thread->alive || waitResult != 0) {

			fileasync->thread->alive = false;
			waitResult = WaitForSingleObject(fileasync->thread->hThread, 100);
		}

		DeleteCriticalSection(&fileasync->thread->CriticalSection);
		CloseHandle(fileasync->thread->hThread);
		gff_free(fileasync->thread->buffer);
		gff_free(fileasync->thread);
		fileasync->thread = NULL;
	}

	return 0;
}

int luafileasync_tostring(lua_State* L) {

	char tim[1024];
	sprintf(tim, "FILEASYNC: 0x%08X", lua_toluafileasync(L, 1));
	lua_pushfstring(L, tim);
	return 1;
}