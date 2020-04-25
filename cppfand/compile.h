#pragma once
#include "access.h"

RdbPos ChptIPos;
void* SaveCompState(); // r104
void RestoreCompState(void* p); // 109
