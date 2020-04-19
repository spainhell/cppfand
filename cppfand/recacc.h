#pragma once
#include "constants.h"
#include "fileacc.h"

void ReadRec(longint N);
pstring _ShortS(FieldDPtr F);
LongStr* _LongS(FieldDPtr F); // r191
bool IsNullValue(void* pointer, WORD l);


longint _T(FieldDescr* F); // r255