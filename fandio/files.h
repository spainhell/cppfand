#pragma once
#include <string>
#include "../Core/FileD.h"


void CopyH(HANDLE h1, HANDLE h2);

void CFileError(FileD* file_d, int N);
void TestCFileError(FileD* file_d);
std::string SetPathMountVolumeSetNet(FileD* file_d, FileUseMode UM);
std::string SetPathAndVolume(FileD* file_d, char pathDelim = '\\');
std::string SetPathForH(HANDLE handle);
