#pragma once
#include "FandFile.h"

class DbfTFile : public FandFile
{
public:
	DbfTFile();
	~DbfTFile() override;

	enum eDbtFormat { DbtFormat, FptFormat } Format = DbtFormat;

	void ClearUpdateFlag() override;

private:
};
