#pragma once
#include <deque>
#include <string>
#include "constants.h"

class FileD;
class LinkD;
struct FuncD;

class RdbD // r. 243
{
public:
	RdbD* ChainBack = nullptr;
	FileD* FD = nullptr;
	FileD* HelpFD = nullptr; // { FD=FileDRoot and = Chpt for this RDB }
	std::deque<LinkD*> OldLDRoot;
	FuncD* OldFCRoot = nullptr;
	bool Encrypted = false;
	std::string RdbDir;
	std::string DataDir;
};

struct RdbPos
{
	RdbD* R = nullptr;
	WORD IRec = 0;
};
