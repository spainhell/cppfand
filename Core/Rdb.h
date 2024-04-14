#pragma once
#include <deque>
#include <string>

class FileD;
class LinkD;
struct FuncD;

class RdbD
{
public:
	RdbD* ChainBack = nullptr;
	FileD* rdb_file = nullptr;
	FileD* help_file = nullptr; // { rdb_file=FileDRoot and = Chpt for this RDB }
	std::deque<LinkD*> OldLDRoot;
	FuncD* OldFCRoot = nullptr;
	bool Encrypted = false;
	std::string RdbDir;
	std::string DataDir;
};

struct RdbPos
{
	RdbD* rdb = nullptr;
	int i_rec = 0;
};
