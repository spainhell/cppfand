#pragma once

//#include "fanddml.h"
#include "access.h"
#include "fileacc.h"

void OpenXWorkH(); // ø. 49
void OpenTWorkH(); // ø. 57
void SaveFD();
void SaveFiles(); // r69
void ClosePassiveFD(); // r76
void CloseFANDFiles(bool FromDML);// ø. 81
void OpenFANDFiles(bool FromDML);// ø. 94
void SetCPathMountVolSetNet(FileUseMode UM);
bool OpenF1(FileUseMode UM); // ø. 119
bool OpenF2(); // ø. 159
bool OpenF(FileUseMode UM); // ø. 196
void CreateF();
bool OpenCreateF(FileUseMode UM); // r218
LockMode RewriteF(bool Append);
void TruncF(); // ø. 239
void CloseFile(); // ø. 252
void CloseFAfter(FileD* FD);
bool ActiveRdbOnDrive(WORD D);
void CloseFilesOnDrive(WORD D);
WORD TestMountVol(char DriveC); // r301
void ReleaseDrive(WORD D);
void SetCPathForH(FILE* handle);
#ifdef FandSQL
void SetIsSQLFile();
#endif
WORD GetCatIRec(pstring Name, bool MultiLevel); // r364
WORD Generation();
void TurnCat(WORD Frst, WORD N, integer I);
pstring RdCatField(WORD CatIRec, FieldDPtr CatF);// ø. 400
void WrCatField(WORD CatIRec, FieldDescr* CatF, pstring Txt);
void RdCatPathVol(WORD CatIRec);
bool SetContextDir(pstring& D, bool& IsRdb);// ø. 414
void GetCPathForCat(WORD I);// ø. 429
void SetCPathVol(); // ø. 441
void SetTxtPathVol(pstring Path, WORD CatIRec); // r463
void SetTempCExt(char Typ, bool IsNet);
FileD* OpenDuplF(bool CrTF);
void CopyDuplF(FileD* TempFD, bool DelTF);
void CopyH(FILE* h1, FILE* h2);
void SubstDuplF(FileD* TempFD, bool DelTF);
void TestDelErr(pstring* P);
void DelDuplF(FileD* TempFD);
