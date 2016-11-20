#pragma once
#include "GffHeader.h"
#include "lua_main_incl.h"

typedef struct GffStruct {
	unsigned int Type;
	unsigned int DataOrDataOffset;
	unsigned int FieldCount;
} GffStruct;

GffStruct * GetStruct(Gff * gff, lua_State *L, unsigned int index);
void PushTopLevelStruct(Gff * gff, lua_State *L);
void PushStruct(Gff * gff, lua_State *L, unsigned int index);
void PushStructFields(Gff * gff, lua_State *L, GffStruct *gffstruct);
size_t CalculateTopLevelStructSize(lua_State*L, Gff* gff);
size_t CalculateStructSize(lua_State*L, Gff* gff);
size_t CalculateStructFields(lua_State*L, Gff* gff);
unsigned int WriteStruct(lua_State*L, Gff* gff);
unsigned int WriteStructFields(lua_State*L, Gff * gff, GffStruct *gffstruct);