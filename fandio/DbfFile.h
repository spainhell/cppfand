#pragma once
#include <string>

#include "FandFile.h"
#include "DbfTFile.h"

class FileD;

class DbfFile : public FandFile
{
public:
	DbfFile(FileD* parent);
	~DbfFile() override;

	DbfTFile* TF = nullptr;

	uint16_t RecLen = 0;
	uint16_t NRecs = 0;
	uint16_t FirstRecPos = 0;

	void WrPrefix();
	void WriteHeader();
	int MakeDbfDcl(std::string& name);

	FileD* GetFileD();

	void ClearUpdateFlag() override;

private:
	FileD* _parent;
	std::string _extToT(const std::string& input_path);
};

