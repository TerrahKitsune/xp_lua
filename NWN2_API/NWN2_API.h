#pragma once
#include "windows.h"

typedef unsigned int nwn_objid_t;

const nwn_objid_t OJBECT_INVALID = 0x7F000000;
const nwn_objid_t OBJECT_MODULE = 0;

#include <stdlib.h>
#include "BaseObjects.h"
#include "CExoString.h"
#include "CNWSObject.h"
#include "CNWSCreature.h"
#include "CGameEffect.h"

void NWN2_InitMem();
void * NWN2_Malloc(size_t size);
void NWN2_Free(void * ptr);