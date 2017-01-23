#pragma once
#include "lua_main_incl.h"
static const char * LERF = "Erf";

typedef struct ERFHeader {
	char FileType[4];
	char Version[4];
	unsigned int LanguageCount;
	unsigned int LocalizedStringSize;
	unsigned int EntryCount;
	unsigned int OffsetToLocalizedString;
	unsigned int OffsetToKeyList;
	unsigned int OffsetToResourceList;
	unsigned int BuildYear;
	unsigned int BuildDay;
	unsigned int DescriptionStrRef;
	unsigned char Reserved[116];
} ERFHeader;

typedef struct ErfLocString {
	unsigned int LanguageID;
	unsigned int StringSize;
	char String[];
}ErfLocString;

typedef struct ErfResList {
	unsigned int OffsetToResource;
	unsigned int ResourceSize;
}ErfResList;

typedef struct ErfKey {
	char ResRef[16];
	unsigned int ResID;
	unsigned short ResType;
	unsigned short Unused;
}ErfKey;

typedef struct ErfKeyV2 {
	char ResRef[32];
	unsigned int ResID;
	unsigned short ResType;
	unsigned short Unused;
}ErfKeyV2;

typedef struct ERF {
	char * File;
	int version;
	ERFHeader * Header;
}ERF;

ERF * lua_pusherf(lua_State *L);
ERF * lua_toerf(lua_State *L, int index);

int OpenErf(lua_State *L);
int GetLocalizedStrings(lua_State *L);
int GetKeys(lua_State *L);
int GetResource(lua_State *L);
int AddFile(lua_State *L);

int erf_gc(lua_State *L);
int erf_tostring(lua_State *L);