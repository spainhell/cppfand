#pragma once
#include "constants.h"
#include "fileacc.h"

void ReadRec(longint N);
string _ShortS(FieldDPtr F);
string _LongS(FieldDPtr F);
bool IsNullValue(void* pointer, WORD l);

