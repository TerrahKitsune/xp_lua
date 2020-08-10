#define _CRT_SECURE_NO_WARNINGS
#include "GffLabel.h"
#include <string.h>

static char LABEL[17];

const char * NullTerminatedLabel(GffLabel * label){

	memset(LABEL, 0, 17);
	memcpy(LABEL, label->Label, 16);
	return LABEL;
}

void PushLabel(lua_State*L, Gff * gff, unsigned int index){

	if (index > gff->Header.LabelCount){
		Bail(gff, L, "GFF Malformed, unable to retrive label");
	}

	GffLabel * label = &((GffLabel*)(&gff->raw[gff->Header.LabelOffset]))[index];
	lua_pushstring(L, NullTerminatedLabel(label));
}

int WriteLabel(lua_State*L, Gff * gff){

	size_t len;
	const char * label = lua_tolstring(L, -1, &len);
	memset(LABEL, 0, 17);
	memcpy(LABEL, label, MIN(len, 16));

	if (gff->Header.LabelOffset >= gff->size || gff->Header.LabelOffset >= gff->Header.FieldDataOffset) {
		Bail(gff, L, "Label offset invalid");
	}
	else if (gff->Header.LabelOffset+(gff->Header.LabelCount * 16) >= gff->size) {
		Bail(gff, L, "Labels are corrupt");
	}

	GffLabel * labels = (GffLabel*)&gff->raw[gff->Header.LabelOffset];
	for (unsigned int n = 0; n < gff->Header.LabelCount; n++){
		if (strncmp(labels[n].Label, LABEL, 16) == 0){
			return n;
		}
	}

	int offset = gff->Header.LabelCount;
	gff->Header.LabelCount++;

	GffLabel * newlabel = &labels[offset];
	memset(newlabel, 0, sizeof(GffLabel));
	memcpy(newlabel, LABEL, len);

	return offset;
}