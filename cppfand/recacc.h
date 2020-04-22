#pragma once
#include "constants.h"
#include "fileacc.h"

void ReadRec(longint N);
void WriteRec(longint N);
pstring _ShortS(FieldDPtr F);
LongStr* _LongS(FieldDPtr F); // r191
bool IsNullValue(void* pointer, WORD l);


double _R(FieldDPtr F); // r226
bool _B(FieldDPtr F); 
longint _T(FieldDescr* F); // r255

void DelAllDifTFlds(void* Rec, void* CompRec); // r376

