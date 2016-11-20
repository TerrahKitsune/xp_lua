#pragma once
#include "lua_main_incl.h"
#include "GffHeader.h"

typedef struct GffLabel {
	char Label[16];
} GffLabel;

const char * NullTerminatedLabel(GffLabel * label);
void PushLabel(lua_State*L, Gff * gff, unsigned int index);
int WriteLabel(lua_State*L, Gff * gff);