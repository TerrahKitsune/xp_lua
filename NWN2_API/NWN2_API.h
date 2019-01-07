#pragma once
#include "windows.h"
#include <stdlib.h>

typedef unsigned int nwn_objid_t;

const nwn_objid_t OJBECT_INVALID = 0x7F000000;
const nwn_objid_t OBJECT_MODULE = 0;

#include "CExoString.h"

struct CNWSObject;					
struct CNWSCreature;				
struct CGameEffect;				
struct CScriptVariable;			
struct Vector;					
struct Location;						
struct CNWSCreatureStats;				

#include "BaseObjects.h"
#include "CNWSObject.h"
#include "CNWSCreature.h"
#include "CGameEffect.h"
#include "CNWSCreatureStats.h"

void NWN2_InitMem();
void * NWN2_Malloc(size_t size);
void NWN2_Free(void * ptr);