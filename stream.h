#pragma once
#include "lua_main_incl.h"
#include <Windows.h>
static const char* STREAM = "STREAM";

typedef struct LuaStream {

	BYTE* data;
	size_t len;
	size_t pos;
	size_t alloc;

} LuaStream;


LuaStream* lua_pushluastream(lua_State* L);
LuaStream* lua_toluastream(lua_State* L, int index);

int StreamBuffer(lua_State* L);
int StreamPos(lua_State* L);
int StreamLen(lua_State* L);
int StreamShrink(lua_State* L);
int NewStream(lua_State *L);
int WriteStreamByte(lua_State *L);
int ReadStreamByte(lua_State *L);
int PeekStreamByte(lua_State* L);
int GetStreamInfo(lua_State *L);
int StreamSetPos(lua_State* L);
int WriteLuaValue(lua_State* L);
int ReadLuaStream(lua_State* L);

int WriteFloat(lua_State* L);
int ReadFloat(lua_State* L);

int WriteDouble(lua_State* L);
int ReadDouble(lua_State* L);

int WriteShort(lua_State* L);
int ReadShort(lua_State* L);

int WriteUShort(lua_State* L);
int ReadUShort(lua_State* L);

int WriteInt(lua_State* L);
int ReadInt(lua_State* L);

int WriteUInt(lua_State* L);
int ReadUInt(lua_State* L);

int WriteLong(lua_State* L);
int ReadLong(lua_State* L);

int WriteUnsignedLong(lua_State* L);
int ReadUnsignedLong(lua_State* L);

int luastream_gc(lua_State* L);
int luastream_tostring(lua_State* L);