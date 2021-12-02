#pragma once

#include "access.h"

void OpenXWorkH();
void OpenTWorkH();
void SaveFD();
void SaveFiles();
void ClosePassiveFD();
void CloseFANDFiles(bool FromDML);
void OpenFANDFiles(bool FromDML);
void SetCPathMountVolSetNet(FileUseMode UM);
void TestCFileError();
bool OpenF1(FileUseMode UM);
bool OpenF2();
bool OpenF(FileUseMode UM);
void CreateF();
bool OpenCreateF(FileUseMode UM);
LockMode RewriteF(bool Append);
void TruncF();
void CloseFile();
void CloseFAfter(FileD* FD);
bool ActiveRdbOnDrive(WORD D);
void CloseFilesOnDrive(WORD D);
WORD TestMountVol(char DriveC);
void ReleaseDrive(WORD D);
void SetCPathForH(FILE* handle);
#ifdef FandSQL
void SetIsSQLFile();
#endif
WORD GetCatIRec(pstring Name, bool MultiLevel);
WORD Generation();
void TurnCat(WORD Frst, WORD N, integer I);
pstring RdCatField(WORD CatIRec, FieldDescr* CatF);
void WrCatField(FileD* catFD, WORD CatIRec, FieldDescr* CatF, const std::string& Txt);
void WrCatField(WORD CatIRec, FieldDescr* CatF, pstring Txt);
void RdCatPathVol(WORD CatIRec);
bool SetContextDir(std::string& D, bool& IsRdb);
void GetCPathForCat(WORD I);
void SetCPathVol();
void SetTxtPathVol(pstring* Path, WORD CatIRec);
void SetTxtPathVol(std::string& Path, WORD CatIRec);
void SetTempCExt(char Typ, bool IsNet);
FileD* OpenDuplF(bool CrTF);
void CopyDuplF(FileD* TempFD, bool DelTF);
void CopyH(FILE* h1, FILE* h2);
void SubstDuplF(FileD* TempFD, bool DelTF);
void TestDelErr(std::string& P);
void DelDuplF(FileD* TempFD);
