#pragma once
#include "lua_main_incl.h"
static const char * TLK = "Tlk";

#define TEXT_PRESENT 0x0001
#define SND_PRESENT 0x0002
#define SNDLENGTH_PRESENT 0x0004

typedef struct TlkStringData {
	unsigned int Flags;
	char SoundResRef[16];
	unsigned int VolumeVariance;
	unsigned int PitchVariance;
	unsigned int OffsetToString;
	unsigned int StringSize;
	float SoundLength;
} TlkStringData;

typedef struct TlkHeader {
	char FileType[4];
	char FileVersion[4];
	unsigned int LanguageID;
	unsigned int StringCount;
	unsigned int StringEntriesOffset;
} TlkHeader;

typedef struct LuaTLK {
	TlkHeader Header;
	FILE * file;
} LuaTLK;


int tlk_open(lua_State *L);
int tlk_getall(lua_State *L);
int tlk_get(lua_State *L);
int tlk_info(lua_State *L);

LuaTLK * lua_pushtlk(lua_State *L);
LuaTLK * lua_totlk(lua_State *L, int index);

int tlk_gc(lua_State *L);
int tlk_tostring(lua_State *L);