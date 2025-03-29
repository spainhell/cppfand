#pragma once
#include <cstdint>
#include <string>

#include "DataFileBase.h"

class DbfFile;

class DbfTFile : public DataFileBase
{
public:
	DbfTFile(DbfFile* parent);
	~DbfTFile() override;

	int FreePart = 0;
	unsigned short FptFormatBlockSize = 0;
	int32_t MaxPage = 0;
	int MLen = 0;
	int LicenseNr = 0;

	enum eDbtFormat { DbtFormat, FptFormat } Format = DbtFormat;

	void Err(unsigned short n, bool ex) const;
	void TestErr() const;
	int UsedFileSize() const;
	void RdPrefix(bool check);
	void WrPrefix();
	void SetEmpty();

	std::string Read(int32_t pos);
	uint32_t Store(const std::string& data);
	void Delete(int32_t pos);

	void Create(const std::string& path);
	void CloseFile();
	void ClearUpdateFlag() override;

private:
	DbfFile* _parent;
	void GetMLen();

	long eofPos = 0;
};

