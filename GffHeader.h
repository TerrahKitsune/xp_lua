#pragma once
#include "lua_main_incl.h"

typedef struct GffHeader {
	char FileType[4];
	char FileVersion[4];
	unsigned int StructOffset;
	unsigned int StructCount;
	unsigned int FieldOffset;
	unsigned int FieldCount;
	unsigned int LabelOffset;
	unsigned int LabelCount;
	unsigned int FieldDataOffset;
	unsigned int FieldDataCount;
	unsigned int FieldIndicesOffset;
	unsigned int FieldIndicesCount;
	unsigned int ListIndicesOffset;
	unsigned int ListIndicesCount;
} GffHeader;

typedef struct StructTrackerLinkedList {
	const void * value;
	StructTrackerLinkedList * next;
} StructTrackerLinkedList;

typedef struct StringLinkedList {
	char * string;
	size_t length;
	int offset;
	StringLinkedList * next;
} StringLinkedList;

typedef struct Gff {
	GffHeader Header;
	size_t size;
	unsigned char * raw;
	StructTrackerLinkedList * gfftracker;
	StructTrackerLinkedList * oneway;
	StringLinkedList * strings;
	size_t stringcount;
} Gff;

//Free the gff memory and pop everything off the stack, runs luaL_error if errormsg isnt NULL
void Bail(Gff * gff, lua_State *L, const char * errormsg);

void * SetGffPointer(size_t typelen, Gff * gff, int offset);
void TrackOrBail(lua_State*L, Gff * gff, const void * gffstruct);
void UntrackOrBail(lua_State*L, Gff * gff, const void * gffstruct);
void UntrackAll(Gff * gff);
bool TrackExists(Gff * gff, const void * data);
int TrackCount(Gff * gff);
void DebugPrintTracker(Gff * gff, const char * str);
void StringAdd(Gff * gff, const char * string, size_t length, unsigned int offset);
int StringExist(Gff * gff, const char * string, size_t length);
void StringClear(Gff * gff);