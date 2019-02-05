#pragma once
typedef unsigned int nwn_objid_t;

const nwn_objid_t OJBECT_INVALID = 0x7F000000;
const nwn_objid_t OBJECT_MODULE = 0;

#include "windows.h"
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