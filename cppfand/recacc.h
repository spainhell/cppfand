#pragma once
#include "constants.h"
#include "fileacc.h"

const WORD Alloc = 2048;

void IncNRecs(longint N);
void DecNRecs(longint N);
void SeekRec(longint N);
void PutRec();
void ReadRec(longint N);
void WriteRec(longint N);
void CreateRec(longint N);
void DeleteRec(longint N);
bool LinkLastRec(FileD* FD, longint& N, bool WithT);
void AsgnParFldFrml(FileD* FD, FieldDPtr F, FrmlPtr Z, bool Ad);
bool SearchKey(XString& XX, KeyDPtr Key, longint& NN);
bool LinkUpw(LinkDPtr LD, longint& N, bool WithT);
void AssignNRecs(bool Add, longint N);

/* FIELD ACCESS */
const double FirstDate = 6.97248E+5;
bool IsNullValue(void* p, WORD l); // ASM
pstring _ShortS(FieldDPtr F);
LongStr* _LongS(FieldDPtr F); // r191
double _RforD(FieldDPtr F, void* P); //r214
double _R(FieldDPtr F); // r226
bool _B(FieldDPtr F); 
longint _T(FieldDescr* F); // r255

void S_(FieldDPtr F, pstring S);
void LongS_(FieldDPtr F, LongStrPtr S);
void R_(FieldDPtr F, double R);
void B_(FieldDPtr F, bool B);
void T_(FieldDPtr F, longint Pos);
void ZeroAllFlds();
void DelTFld(FieldDPtr F);
void DelDifTFld(void* Rec, void* CompRec, FieldDPtr F);
void ClearRecSpace(void* p);
void DelAllDifTFlds(void* Rec, void* CompRec); // r376
void DelTFlds();
void CopyRecWithT(void* p1, void* p2);

