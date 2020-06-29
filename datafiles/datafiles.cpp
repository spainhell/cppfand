#include "pch.h"
#include "datafiles.h"

std::map<std::string, DataFile> filesMap;

DataFile::DataFile()
{
	state = _closed;
}

DataFile::DataFile(std::string Name, FileD* CFile, FILE* Handler)
{
	this->Name = Name;
	this->CFile = CFile;
	this->Handler = Handler;
	if (Handler != nullptr) state = _opened;
}

void DataFile::SetClose()
{
	state = _closed;
	Handler = nullptr;
}

void DataFile::SetOpen()
{
	state = _opened;
}
