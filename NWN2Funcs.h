#pragma once
#include "./NWN2_API/NWN2_API.h"

CNWSCreature * GetCreatureByGameObjectID(nwn_objid_t objectid);
CNWSGenericObject * GetObjectByGameObjectID(nwn_objid_t objectid);
CNWSCreatureStats* GetCreatureStats(CNWSObject * obj);
void RunScript(const char * script, nwn_objid_t obj);