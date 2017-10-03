#pragma once
#include "./NWN2_API/CNWSCreature.h"

typedef unsigned int nwn_objid_t;

const nwn_objid_t OJBECT_INVALID = 0x7F000000;
const nwn_objid_t OBJECT_MODULE = 0;

CNWSCreature * GetCreatureByGameObjectID(nwn_objid_t objectid);

void RunScript(const char * script, nwn_objid_t obj);