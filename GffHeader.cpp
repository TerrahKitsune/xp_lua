#define _CRT_SECURE_NO_WARNINGS
#include "GffHeader.h"
#include <stdlib.h>
#include <string.h>
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void Bail(Gff * gff, lua_State *L, const char * errormsg){

	lua_pop(L, lua_gettop(L));
	UntrackAll(gff);
	StringClear(gff);
	gff_free(gff->raw);
	gff_free(gff);
	if (errormsg != NULL)
		luaL_error(L, errormsg);
}



void StringAdd(Gff * gff, const char * string, size_t length, unsigned int offset){

	StringLinkedList ** node = NULL;
	StringLinkedList * traversal = NULL;

	if (gff->strings == NULL){
		node = &gff->strings;
	}
	else{
		traversal = gff->strings;
		while (traversal){
			node = &traversal->next;
			traversal = traversal->next;
		}
	}

	(*node) = (StringLinkedList *)gff_malloc(sizeof(StringLinkedList));
	if ((*node) == NULL)
		return;
	(*node)->length = length;
	(*node)->string = (char*)gff_malloc(length + 1);
	if ((*node)->string == NULL){
		gff_free((*node));
		(*node) = NULL;
		return;
	}
	(*node)->string[length] = '\0';
	strncpy((*node)->string, string, length);
	(*node)->length = length;
	(*node)->offset = offset;
	(*node)->next = NULL;
	gff->stringcount++;
}

int StringExist(Gff * gff, const char * string, size_t length){
	StringLinkedList * traversal = gff->strings;
	while (traversal){
		if (traversal->length == length && strncmp(traversal->string, string, length) == 0){
			return traversal->offset;
		}
		traversal = traversal->next;
	}
	return -1;
}

void StringClear(Gff * gff){
	StringLinkedList * traversal = gff->strings;
	StringLinkedList * remove = NULL;
	while (traversal){
		remove = traversal;
		traversal = traversal->next;
		if (remove->string)
			gff_free(remove->string);
		gff_free(remove);
	}
	gff->strings = NULL;
	gff->stringcount = 0;
}

bool TrackExists(Gff * gff, const void * data){
	StructTrackerLinkedList * Traverse = gff->gfftracker;
	while (Traverse){
		if (Traverse->value == data)
			return true;
		Traverse = Traverse->next;
	}

	return false;
}

void DebugPrintTracker(Gff * gff, const char * str){

	StructTrackerLinkedList * Traverse = gff->gfftracker;
	while (Traverse){
		printf("0x%08X -> ", Traverse->value);
		Traverse = Traverse->next;
	}

	printf("%s\n", str);
}

void * SetGffPointer(size_t typelen, Gff * gff, int offset){

	if (offset + typelen > gff->size){
		return NULL;
	}

	return &gff->raw[offset];
}

int TrackCount(Gff * gff){

	int cnt = 0;
	StructTrackerLinkedList * Traverse = gff->oneway;
	while (Traverse){
		cnt++;
		Traverse = Traverse->next;
	}
	return cnt;
}

void TrackOrBail(lua_State*L, Gff * gff, const void * gffstruct){

	if (gff->gfftracker == NULL){
		gff->gfftracker = (StructTrackerLinkedList*)gff_malloc(sizeof(StructTrackerLinkedList));
		if (!gff->gfftracker) {
			Bail(gff, L, "No memory");
		}
		gff->gfftracker->next = NULL;
		gff->gfftracker->value = gffstruct;
	}
	else{
		StructTrackerLinkedList * Parent = NULL;
		StructTrackerLinkedList * Traverse = gff->gfftracker;
		while (Traverse){

			if (Traverse->value == gffstruct)
				Bail(gff, L, "GFF Malformed, circular structs detected");

			Parent = Traverse;
			Traverse = Traverse->next;
		}

		Parent->next = (StructTrackerLinkedList*)gff_malloc(sizeof(StructTrackerLinkedList));
		if (!Parent->next) {
			Bail(gff, L, "No memory");
		}
		Parent->next->next = NULL;
		Parent->next->value = gffstruct;
	}
}

void UntrackOrBail(lua_State*L, Gff * gff, const void * gffstruct){

	StructTrackerLinkedList * Parent = NULL;
	StructTrackerLinkedList * Traverse = gff->gfftracker;

	while (Traverse){

		if (Traverse->value == gffstruct)
			break;

		Parent = Traverse;
		Traverse = Traverse->next;
	}

	if (Traverse == NULL || Traverse->next != NULL){
		Bail(gff, L, "GFF Malformed, invalid order of struct loading");
	}
	else if (Parent == NULL){
		if (gff->gfftracker->value != gffstruct || gff->gfftracker->next != NULL)
			Bail(gff, L, "GFF Malformed, invalid order of struct loading");
		else {
			gff_free(gff->gfftracker);
			gff->gfftracker = NULL;
		}
	}
	else if (Parent->next == NULL || Parent->next->value != gffstruct){
		Bail(gff, L, "GFF Malformed, invalid order of struct loading");
	}
	else{
		gff_free(Parent->next);
		Parent->next = NULL;
	}
}

void UntrackAll(Gff * gff){

	StructTrackerLinkedList * Parent = NULL;
	StructTrackerLinkedList * Traverse = gff->gfftracker;

	while (Traverse){

		if (Traverse->next == NULL){

			if (Parent == NULL){
				gff_free(gff->gfftracker);
				gff->gfftracker = NULL;
				return;
			}
			else{
				gff_free(Parent->next);
				Parent->next = NULL;
			}

			Parent = NULL;
			Traverse = gff->gfftracker;
		}
		else{
			Parent = Traverse;
			Traverse = Traverse->next;
		}
	}
}