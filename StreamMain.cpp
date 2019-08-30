#include "stream.h"
#include "StreamMain.h"

static const struct luaL_Reg streamfunctions[] = {
	{ "Close",  luastream_gc },
	{ "Create",  NewStream },
	{ "Open",  OpenFileToStream },
	{ "WriteToFile",  WriteToFile },
	{ "ReadFromFile",  ReadFromFile },
	{ "Save",  DumpToFile },
	{ "len",  StreamLen },
	{ "pos",  StreamPos },
	{ "WriteByte",  WriteStreamByte },
	{ "ReadByte",  ReadStreamByte },
	{ "SetByte",  SetStreamByte },
	{ "PeekByte",  PeekStreamByte },
	{ "GetInfo",  GetStreamInfo },
	{ "Shrink",  StreamShrink },
	{ "Seek",  StreamSetPos },
	{ "Buffer",  StreamBuffer },
	{ "Write",  WriteLuaValue },
	{ "Read",  ReadLuaStream },
	{ "WriteFloat",  WriteFloat },
	{ "ReadFloat",  ReadFloat },
	{ "WriteDouble",  WriteDouble },
	{ "ReadDouble",  ReadDouble },
	{ "WriteShort",  WriteShort },
	{ "ReadShort",  ReadShort },
	{ "WriteUnsignedShort",  WriteUShort },
	{ "ReadUnsignedShort",  ReadUShort },
	{ "WriteInt",  WriteInt },
	{ "ReadInt",  ReadInt },
	{ "WriteUnsignedInt",  WriteUInt },
	{ "ReadUnsignedInt",  ReadUInt },
	{ "WriteLong",  WriteLong },
	{ "ReadLong",  ReadLong },
	{ "WriteUnsignedLong",  WriteUnsignedLong },
	{ "ReadUnsignedLong",  ReadUnsignedLong },
	{ NULL, NULL }
};

static const luaL_Reg streammeta[] = {
	{ "__gc",  luastream_gc },
	{ "__tostring",  luastream_tostring },
	{ NULL, NULL }
};

int luaopen_stream(lua_State* L) {

	luaL_newlibtable(L, streamfunctions);
	luaL_setfuncs(L, streamfunctions, 0);

	luaL_newmetatable(L, STREAM);
	luaL_setfuncs(L, streammeta, 0);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_pop(L, 1);
	return 1;
}