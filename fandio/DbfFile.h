#pragma once
#include <string>

#include "FandFile.h"
#include "DbfTFile.h"
#include "../Core/FieldDescr.h"

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
	bool WasRdOnly = false;

	bool loadB(FieldDescr* field_d, void* record);
	double loadR(FieldDescr* field_d, void* record);
	std::string loadS(FieldDescr* field_d, void* record);
	int loadT(FieldDescr* F, void* record);

	void saveB(FieldDescr* field_d, bool b, void* record);
	void saveR(FieldDescr* field_d, double r, void* record);
	void saveS(FileD* parent, FieldDescr* field_d, std::string s, void* record);
	int saveT(FieldDescr* field_d, int pos, void* record);

	uint16_t RdPrefix();
	void WrPrefix();
	void WrPrefixes();
	void WriteHeader();
	int MakeDbfDcl(std::string& name);

	int UsedFileSize() const;

	void TruncFile();
	void CloseFile();

	bool DeletedFlag(void* record);
	void ClearDeletedFlag(void* record);
	void SetDeletedFlag(void* record);

	FileD* GetFileD();

	void ClearUpdateFlag() override;

private:
	FileD* _parent;
	double DBF_RforD(FieldDescr* field_d, uint8_t* source);
	std::string _extToT(const std::string& input_path);
};
