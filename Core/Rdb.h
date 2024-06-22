#pragma once
#include <deque>
#include <string>
#include <vector>

class FileD;
class LinkD;
struct FuncD;

class RdbD
{
public:
	RdbD* ChainBack = nullptr;
	std::vector<FileD*> v_files;
	FileD* help_file = nullptr; // { v_files=FileDRoot and = Chpt for this RDB }
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
