#pragma once
#include "CGameEffect.h"

#define CGameObject__OBJECT_TYPE_GUI 0x1
#define CGameObject__OBJECT_TYPE_TILE 0x2
#define CGameObject__OBJECT_TYPE_MODULE 0x3
#define CGameObject__OBJECT_TYPE_AREA 0x4
#define CGameObject__OBJECT_TYPE_CREATURE 0x5
#define CGameObject__OBJECT_TYPE_ITEM 0x6
#define CGameObject__OBJECT_TYPE_TRIGGER 0x7
#define CGameObject__OBJECT_TYPE_PROJECTILE 0x8
#define CGameObject__OBJECT_TYPE_PLACEABLE 0x9
#define CGameObject__OBJECT_TYPE_DOOR 0xA
#define CGameObject__OBJECT_TYPE_AREAOFEFFECTOBJECT 0xB
#define CGameObject__OBJECT_TYPE_WAYPOINT 0xC
#define CGameObject__OBJECT_TYPE_ENCOUNTER 0xD
#define CGameObject__OBJECT_TYPE_STORE 0xE
#define CGameObject__OBJECT_TYPE_PORTAL 0xF
#define CGameObject__OBJECT_TYPE_SOUND 0x10

struct CNWSGenericObject {
	void * functionarray; //0
	nwn_objid_t obj; //4
	BYTE padding8[124]; //8
	char * base; //84
	DWORD dword88; //88
	DWORD dword8C; //8C
	DWORD dword90; //90
	DWORD dword94; //94
	DWORD dword98; //98
	DWORD dword9c; //9C
	nwn_objid_t ObjectId; //A0
};

struct CNWSVarTable {
	CScriptVariable * vartable; //190
	int vartable_len; //194
	int vartable_alloc; //198
};

struct CNWSObject{
	CNWSGenericObject GenericObj;
	DWORD dwordPadding;
	BYTE byteA4; //A4
	BYTE byteA5;
	BYTE byteA6;
	BYTE ObjectType;
	DWORD dwordA8; //A8
	DWORD dwordAC; //AC
	void * obj_vtable; //B0
	char * extraname; //B4
	nwn_objid_t objidB8; //B8
	DWORD dwordBC; //BC
	CExoString vfx; //C0
	DWORD dwordC8; //C8
	DWORD dwordCC; //CC
	void *ptrD0; //D0
	DWORD dwordD4; //D4
	CExoString portrait; //D8
	CExoString portraitsecond; //E0
	DWORD dwordE8; //E8
	DWORD dwordEC; //EC
	DWORD dwordF0; //F0
	DWORD dwordF4; //F4
	DWORD dwordF8; //F8
	DWORD dwordFC; //FC
	DWORD dword100; //100
	DWORD dword104; //104
	DWORD dword108; //108
	DWORD dword10C; //10C
	DWORD dword110; //110
	DWORD dword114; //114
	DWORD dword118; //118
	DWORD dword11C_AISomething; //11C
	DWORD dword120; //120
	DWORD dword124; //124
	nwn_objid_t objid128; //128
	DWORD dword12C; //12C
	DWORD dword130; //130
	DWORD dword134; //134
	DWORD dword138; //138
	DWORD dword13C; //13C
	nwn_objid_t objid140; //140
	DWORD dword144; //144
	DWORD dword148; //148
	DWORD dword14C; //14C
	DWORD dword150; //150
	DWORD dword154; //154
	DWORD dword158; //158
	DWORD dword15C; //15C
	WORD word160; //160
	short CurrentHp;
	WORD word164; //164
	short MaxHp;
	WORD word16C; //16C
	short TempHp;
	DWORD obj_is_commandable; //170
	DWORD obj_is_destroyable; //174
	DWORD obj_is_raisable; //178
	DWORD obj_is_dead_selectable; //17C
	DWORD obj_is_invulnerable; //180
	void * action; //184
	DWORD dword188; //188
	int ActionRounds; //18C
	CNWSVarTable vartable; //190
	//int vartable_len; //194
	//int vartable_alloc; //198
	void * ptr19C; //19C
	CGameEffect ** effects; //1A0
	int effects_len; //1A4
	int effects_alloc; //1A8
};