#pragma once
#include "lua_main_incl.h"
#include "NWN2Funcs.h"
#include "./NWN2_API/NWN2_API.h"

static int(__fastcall *OriginalGetAttackModifierVersus)(CNWSCreatureStats * pThis, void*, CNWSObject *target) = (int(__fastcall *)(CNWSCreatureStats *, void*, CNWSObject *))0x5AEFE0;

int HookResolveBonusCombatDamage(lua_State*L);
int HookDisconnectPlayer(lua_State*L);