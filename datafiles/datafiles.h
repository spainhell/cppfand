#pragma once
#include <cstdio>
#include <string>
#include <map>

class FileD;
enum State { _closed = 0, _opened = 1 };

class DataFile {
public:
	DataFile();
	DataFile(std::string Name, FileD* CFile, FILE* Handler);

	FILE* Handler = nullptr;
	FileD* CFile = nullptr;
	std::string Name;
	std::string FullName;
	State state;
	void SetClose();
	void SetOpen();
};

extern std::map<std::string, DataFile> filesMap;
