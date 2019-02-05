#pragma once
#include "BaseObjects.h"

struct CNWSVarTable {
	CScriptVariable * vartable; //190
	int vartable_len; //194
	int vartable_alloc; //198
};

void ClearVariables(CNWSVarTable * vars);