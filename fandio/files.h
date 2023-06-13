#pragma once
#include <string>
#include "../CppFand/FileD.h"

bool OpenF(FileD* file_d, const std::string& path, FileUseMode UM);
bool OpenF1(FileD* file_d, const std::string& path, FileUseMode UM);
bool OpenF2(FileD* file_d, const std::string& path);
void CreateF(FileD* file_d);
bool OpenCreateF(FileD* file_d, const std::string& path, FileUseMode UM);
void CopyH(HANDLE h1, HANDLE h2);

void CFileError(FileD* file_d, int N);
void TestCFileError(FileD* file_d);
std::string SetPathMountVolumeSetNet(FileD* file_d, FileUseMode UM);
std::string SetPathAndVolume(FileD* file_d, char pathDelim = '\\');
std::string SetPathForH(HANDLE handle);

std::string SetTempCExt(FileD* file_d, char typ, bool isNet);