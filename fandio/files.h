#pragma once
#include <string>
#include "../cppfand/FileD.h"

bool OpenF(FileD* file_d, const std::string& path, FileUseMode UM);
bool OpenF1(FileD* file_d, const std::string& path, FileUseMode UM);
bool OpenF2(FileD* file_d, const std::string& path);

void CFileError(FileD* file_d, int N);
void TestCFileError(FileD* file_d);
void SetCPathMountVolSetNet(FileD* file_d, FileUseMode UM);