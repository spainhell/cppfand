#pragma once

#include "access.h"

void OpenXWorkH();
void OpenTWorkH();

void SaveFiles();
void CloseFANDFiles(bool FromDML);
void OpenFANDFiles(bool FromDML);


//void CloseFilesAfter(FileD* first_for_close, std::vector<FileD*>& v_files);
bool ActiveRdbOnDrive(WORD D);
void CloseFilesOnDrive(WORD drive);
WORD TestMountVol(char DriveC);
void ReleaseDrive(WORD D);

#ifdef FandSQL
void SetIsSQLFile();
#endif

bool SetContextDir(FileD* file_d, std::string& dir, bool& isRdb);

void SetTxtPathVol(std::string& Path, int CatIRec);

void TestDelErr(std::string& P);
