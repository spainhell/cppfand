#pragma once
#include <deque>
#include <string>
#include <vector>

class FileD;
class LinkD;
struct FuncD;

class Project
{
public:
	Project* ChainBack = nullptr;
	FileD* project_file = nullptr;	// RDB project file
	FileD* help_file = nullptr;		// HLP files
	std::vector<FileD*> data_files; // data files
	std::deque<LinkD*> OldLDRoot;
	std::deque<FuncD*> OldFCRoot;
	bool Encrypted = false;
	std::string RdbDir;
	std::string DataDir;
};
