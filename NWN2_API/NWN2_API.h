#pragma once

#include <stdlib.h>
#include "CExoString.h"

void NWN2_InitMem();
void * NWN2_Malloc(size_t size);
void NWN2_Free(void * ptr);