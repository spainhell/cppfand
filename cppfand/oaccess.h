#pragma once

#include "access.h"

void OpenXWorkH();
void OpenTWorkH();

void SaveAndCloseAllFiles();
void ClosePassiveFD();
void CloseFANDFiles(bool FromDML);
void OpenFANDFiles(bool FromDML);


void CloseFilesAfter(FileD* FD);
bool ActiveRdbOnDrive(WORD D);
void CloseFilesOnDrive(WORD D);
WORD TestMountVol(char DriveC);
void ReleaseDrive(WORD D);

#ifdef FandSQL
void SetIsSQLFile();
#endif

bool SetContextDir(FileD* file_d, std::string& dir, bool& isRdb);

void SetTxtPathVol(std::string& Path, int CatIRec);

void TestDelErr(std::string& P);
