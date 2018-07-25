#pragma once
#include "NWN2_API.h"
#include "CExoString.h"

struct CScriptVariable {
	CExoString Name;
	DWORD Type;
	DWORD Data;
};

struct Vector {
	float x;
	float y;
	float z;
};

struct Location {
	Vector Position;
	Vector Facing;
	nwn_objid_t AreaId;
};