#ifdef _DEBUG

#include "datafiles.h"

std::map<std::string, DataFile> filesMap;

DataFile::DataFile()
{
	state = _closed;
}

DataFile::DataFile(const std::string& name, FileD* file_d, FILE* handle)
{
	this->name = name;
	this->file_d = file_d;
	this->handle = handle;
	if (handle != nullptr) state = _opened;
}

void DataFile::SetClose()
{
	state = _closed;
	handle = nullptr;
}

void DataFile::SetOpen()
{
	state = _opened;
}

#endif