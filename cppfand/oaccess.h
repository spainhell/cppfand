#pragma once

//#include "fanddml.h"
#include "access.h"
#include "fileacc.h"

void OpenXWorkH(); // ø. 49
void OpenTWorkH(); // ø. 57
void SaveFiles(); // r69
void ClosePassiveFD(); // r76
void CloseFANDFiles(bool FromDML);// ø. 81
void OpenFANDFiles(bool FromDML);// ø. 94
bool OpenF1(FileUseMode UM);// ø. 119
bool OpenF2();// ø. 159
bool OpenF(FileUseMode UM);// ø. 196
bool OpenCreateF(FileUseMode UM); // r218
void TruncF();// ø. 239
void CloseFile();// ø. 252
WORD TestMountVol(char DriveC); // r301
WORD GetCatIRec(pstring Name, bool MultiLevel); // r364
pstring RdCatField(WORD CatIRec, FieldDPtr CatF);// ø. 400
bool SetContextDir(pstring& D, bool& IsRdb);// ø. 414
void GetCPathForCat(WORD I);// ø. 429
void SetCPathVol(); // ø. 441
void SetTxtPathVol(pstring Path, WORD CatIRec); // r463
