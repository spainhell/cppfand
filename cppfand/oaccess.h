#pragma once

//#include "fanddml.h"
#include "access.h"
#include "fileacc.h"

// ø. 49
void OpenXWorkH();

// ø. 57
void OpenTWorkH();

// ø. 81
void CloseFANDFiles(bool FromDML);

// ø. 94
void OpenFANDFiles(bool FromDML);

// ø. 119
bool OpenF1(FileUseMode UM);

// ø. 159
bool OpenF2();

// ø. 196
bool OpenF(FileUseMode UM);

// ø. 239
void TruncF();

// ø. 252
void CloseFile();

WORD GetCatIRec(pstring Name, bool MultiLevel); // r364

// ø. 400
string RdCatField(WORD CatIRec, FieldDPtr CatF);

// ø. 414
bool SetContextDir(pstring& D, bool& IsRdb);

// ø. 429
void GetCPathForCat(WORD I);

// ø. 441
void SetCPathVol();
