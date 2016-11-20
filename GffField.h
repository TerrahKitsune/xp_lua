#pragma once
#include "GffStruct.h"
#include "lua_main_incl.h"

typedef struct GffField {
	unsigned int Type;
	unsigned int LabelIndex;
	unsigned int DataOrDataOffset;
} GffField;

typedef struct CExoString {
	unsigned int Length;
	char data[];
} CExoString;

typedef struct ResRef {
	unsigned char Length;
	char data[16];
} ResRef;

typedef struct  CExoLocStringSubString {
	int StringID;
	int StringLength;
	char Data[];
} CExoLocStringSubString;

typedef struct CExoLocString {
	unsigned int TotalSize;
	unsigned int StringRef;
	unsigned int StringCount;
	char Strings[];
} CExoLocString;

typedef struct StructList {
	unsigned int Length;
	unsigned int StructIndecies[];
} StructList;

//Pushes the label followed by a table containing the field info to the stack
void PushField(lua_State *L, Gff * gff, unsigned int fieldindex);
void PushFieldData(lua_State *L, Gff * gff, GffField *gfffield);
void PushStructList(lua_State *L, Gff * gff, unsigned int offset);
void CopyFromData(lua_State *L, Gff * gff, unsigned offset, void * dst, size_t size);
void * GetPtrFromData(lua_State *L, Gff * gff, unsigned offset, size_t size);
void PushCExoLocString(lua_State *L, CExoLocString * locstr, Gff * gff, unsigned int originaloffset);
size_t CalculateFieldSize(lua_State *L, Gff * gff);
size_t CalculateFieldDataSize(lua_State *L, Gff * gff, int type, const char * label);
unsigned int WriteField(lua_State *L, Gff * gff);
unsigned int WriteFieldData(lua_State *L, Gff * gff, unsigned int type);