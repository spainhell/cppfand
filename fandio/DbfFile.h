#pragma once
#include <string>

#include "DataFileBase.h"
#include "DbfTFile.h"
#include "FieldDescr.h"

class FileD;

class DbfFile : public DataFileBase
{
public:
	DbfFile(FileD* parent);
	~DbfFile() override;

	DbfTFile* TF = nullptr;
	FileUseMode UMode = Closed;

	uint16_t RecLen = 0;
	uint16_t NRecs = 0;
	uint16_t FirstRecPos = 0;
	unsigned char Drive = 0;           // 1=A, 2=B, else 0
	bool WasWrRec = false;
	bool WasRdOnly = false;
	bool Eof = false;

	size_t ReadRec(size_t rec_nr, uint8_t* record);
	size_t WriteRec(size_t rec_nr, uint8_t* record);
	void CreateRec(int n, uint8_t* record);
	void DeleteRec(int n, uint8_t* record);
	void DelAllDifTFlds(uint8_t* record, uint8_t* comp_record);

	void IncNRecs(int n);
	void DecNRecs(int n);
	void PutRec(uint8_t* record, int& i_rec);

	bool loadB(FieldDescr* field_d, uint8_t* record);
	double loadR(FieldDescr* field_d, uint8_t* record);
	std::string loadS(FieldDescr* field_d, uint8_t* record);
	int loadT(FieldDescr* F, uint8_t* record);

	void saveB(FieldDescr* field_d, bool b, uint8_t* record);
	void saveR(FieldDescr* field_d, double r, uint8_t* record);
	void saveS(FileD* parent, FieldDescr* field_d, std::string s, uint8_t* record);
	int saveT(FieldDescr* field_d, int pos, uint8_t* record);

	void DelTFld(FieldDescr* field_d, uint8_t* record);
	void DelTFlds(uint8_t* record);
	void DelDifTFld(FieldDescr* field_d, uint8_t* record, uint8_t* comp_record);

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
	void Close();

	void SetTWorkFlag(uint8_t* record) const;
	bool HasTWorkFlag(uint8_t* record) const;

	void SetRecordUpdateFlag(uint8_t* record) const;
	void ClearRecordUpdateFlag(uint8_t* record) const;
	bool HasRecordUpdateFlag(uint8_t* record) const;

	static bool DeletedFlag(uint8_t* record);
	void ClearDeletedFlag(uint8_t* record);
	void SetDeletedFlag(uint8_t* record);

	FileD* GetFileD();

	void ClearUpdateFlag() override;
	std::string SetTempCExt(char typ, bool isNet) const;

private:
	FileD* _parent;
	double DBF_RforD(FieldDescr* field_d, uint8_t* source);
	std::string _extToT(const std::string& input_path);
};
