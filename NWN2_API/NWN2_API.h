#pragma once
#include "windows.h"
#include <stdlib.h>

struct CNWSObject;					
struct CNWSCreature;				
struct CGameEffect;			
struct CNWSVarTable;
struct CScriptVariable;			
struct Vector;					
struct Location;						
struct CNWSCreatureStats;				

#include "BaseObjects.h"
#include "CNWSVarTable.h"
#include "CNWSObject.h"
#include "CNWSCreature.h"
#include "CGameEffect.h"
#include "CNWSCreatureStats.h"

void NWN2_InitMem();
void * NWN2_Malloc(size_t size);
void NWN2_Free(void * ptr);