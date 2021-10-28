#pragma once
#include <string>
#include "constants.h"

class FileD;
struct LinkD;
struct FuncD;

class RdbD // r. 243
{
public:
	RdbD* ChainBack = nullptr;
	FileD* FD = nullptr;
	FileD* HelpFD = nullptr; // { FD=FileDRoot and = Chpt for this RDB }
	LinkD* OldLDRoot = nullptr;
	FuncD* OldFCRoot = nullptr;
	// void* Mark2 = nullptr; // { markstore2 at beginning }
	bool Encrypted = false;
	std::string RdbDir;
	std::string DataDir;
};

struct RdbPos // r. 113
{
	RdbD* R = nullptr;
	WORD IRec = 0;
};
