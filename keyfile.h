#pragma once
#include "lua_main_incl.h"
#include "windows.h"
#include <cstdio>

typedef struct KeyHeader {

	char FileType[4];
	char FileVersion[4];
	DWORD BIFCount;
	DWORD KeyCount;
	DWORD OffsetToFileTable;
	DWORD OffsetToKeyTable;
	DWORD BuildYear;
	DWORD BuildDay;
	BYTE Reserved[32];

} KeyHeader;

typedef struct KeyFile {

	KeyHeader Header;
	FILE* file;

} KeyFile;

KeyFile * OpenKeyFile(const char* filename);
void CloseKeyFile(KeyFile * file);