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

bool OpenF(const std::string& path, FileUseMode UM);
bool OpenF1(const std::string& path, FileUseMode UM);
bool OpenF2(const std::string& path);

void CreateF();
bool OpenCreateF(FileD* fileD, FileUseMode UM);
LockMode RewriteF(bool Append);
void TruncF();
void CloseFile();
void CloseFilesAfter(FileD* FD);
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
void TurnCat(WORD Frst, WORD N, short I);
std::string RdCatField(FileD* catalog, WORD cat_IRec, FieldDescr* cat_field);
void WrCatField(FileD* catalog, WORD CatIRec, FieldDescr* CatF, const std::string& Txt);
void RdCatPathVol(WORD CatIRec);
bool SetContextDir(FileD* file_d, std::string& D, bool& IsRdb);
void GetCPathForCat(WORD I);
std::string SetCPathVol(FileD* file_d, char pathDelim = '\\');
void SetTxtPathVol(pstring* Path, WORD CatIRec);
void SetTxtPathVol(std::string& Path, WORD CatIRec);
void SetTempCExt(char Typ, bool IsNet);
FileD* OpenDuplF(bool CrTF);
void CopyDuplF(FileD* TempFD, bool DelTF);
void CopyH(FILE* h1, FILE* h2);
void SubstDuplF(FileD* TempFD, bool DelTF);
void TestDelErr(std::string& P);
void DelDuplF(FileD* TempFD);
