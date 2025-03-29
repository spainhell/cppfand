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
	bool WasWrRec = false;
	bool WasRdOnly = false;
	bool Eof = false;

	size_t ReadRec(size_t rec_nr, void* record);
	size_t WriteRec(size_t rec_nr, void* record);
	void CreateRec(int n, void* record);
	void DeleteRec(int n, void* record);
	void DelAllDifTFlds(void* record, void* comp_record);

	void IncNRecs(int n);
	void DecNRecs(int n);
	void PutRec(void* record, int& i_rec);

	bool loadB(FieldDescr* field_d, void* record);
	double loadR(FieldDescr* field_d, void* record);
	std::string loadS(FieldDescr* field_d, void* record);
	int loadT(FieldDescr* F, void* record);

	void saveB(FieldDescr* field_d, bool b, void* record);
	void saveR(FieldDescr* field_d, double r, void* record);
	void saveS(FileD* parent, FieldDescr* field_d, std::string s, void* record);
	int saveT(FieldDescr* field_d, int pos, void* record);

	void DelTFld(FieldDescr* field_d, void* record);
	void DelTFlds(void* record);
	void DelDifTFld(FieldDescr* field_d, void* record, void* comp_record);

	uint16_t RdPrefix();
	void WrPrefix();
	void WrPrefixes();
	void WriteHeader();
	int MakeDbfDcl(std::string& name);

	void CompileRecLen();
	int UsedFileSize() const;

	void TruncFile();
	void SaveFile();
	void CloseFile();

	void SetTWorkFlag(void* record) const;
	bool HasTWorkFlag(void* record) const;

	void SetRecordUpdateFlag(void* record);
	void ClearRecordUpdateFlag(void* record);
	bool HasRecordUpdateFlag(void* record);

	bool DeletedFlag(void* record);
	void ClearDeletedFlag(void* record);
	void SetDeletedFlag(void* record);

	FileD* GetFileD();

	void ClearUpdateFlag() override;
	std::string SetTempCExt(char typ, bool isNet) const;

private:
	FileD* _parent;
	double DBF_RforD(FieldDescr* field_d, uint8_t* source);
	std::string _extToT(const std::string& input_path);
};
