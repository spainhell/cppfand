#pragma once

#ifdef _DEBUG

#include <cstdio>
#include <string>
#include <map>

class FileD;
enum State { _closed = 0, _opened = 1 };

class DataFile {
public:
	DataFile();
	DataFile(const std::string& name, FileD* file_d, FILE* handle);

	FILE* handle = nullptr;
	FileD* file_d = nullptr;
	std::string name;
	std::string full_name;
	State state;
	void SetClose();
	void SetOpen();
};

extern std::map<std::string, DataFile> filesMap;

#endif