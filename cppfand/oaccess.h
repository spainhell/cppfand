#pragma once

#include "access.h"

void OpenXWorkH();
void OpenTWorkH();

void SaveFiles();
void ClosePassiveFD();
void CloseFANDFiles(bool FromDML);
void OpenFANDFiles(bool FromDML);
void SetCPathMountVolSetNet(FileUseMode UM);
void TestCFileError(FileD* file_d);

bool OpenF(FileD* file_d, const std::string& path, FileUseMode UM);
bool OpenF1(FileD* file_d, const std::string& path, FileUseMode UM);
bool OpenF2(FileD* file_d, const std::string& path);

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
int GetCatalogIRec(const std::string& name, bool multilevel);
WORD Generation();
void TurnCat(WORD Frst, WORD N, short I);
bool SetContextDir(FileD* file_d, std::string& D, bool& IsRdb);
void GetCPathForCat(int i);
std::string SetCPathVol(FileD* file_d, char pathDelim = '\\');
void SetTxtPathVol(std::string& Path, int CatIRec);
void SetTempCExt(char Typ, bool IsNet);
FileD* OpenDuplF(bool CrTF);
void CopyDuplF(FileD* TempFD, bool DelTF);
void CopyH(FILE* h1, FILE* h2);
void SubstDuplF(FileD* TempFD, bool DelTF);
void TestDelErr(std::string& P);
void DelDuplF(FileD* TempFD);
