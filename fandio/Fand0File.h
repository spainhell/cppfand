#pragma once
#include <map>
#include <memory>

#include "FandTFile.h"
#include "FandXFile.h"
#include "../Core/switches.h"
#include "FieldDescr.h"
#include "ProgressCallbacks.h"
#include "../Logging/Logging.h"
#include "../Common/OperationType.h"

class Record;
class XWKey;
class XScan;
class FileD;

enum class FandFileType
{
	UNKNOWN,
	FAND8,
	FAND16,
	INDEX,
	RDB,
	CAT
};

class Fand0File : public DataFileBase
{
public:
	Fand0File(FileD* parent, ProgressCallbacks callbacks);
	Fand0File(const Fand0File& orig) = delete;
	Fand0File(const Fand0File& orig, FileD* parent);
	~Fand0File() override;

	std::unique_ptr<uint8_t[]> GetRecSpaceUnique() const;

	unsigned short RecLen = 0;
	//Record* RecPtr = nullptr; // Merge & Report dependency
	int32_t NRecs = 0;
	bool WasWrRec = false;
	bool WasRdOnly = false;
	bool Eof = false;
	FandFileType file_type = FandFileType::UNKNOWN;       // 8 = Fand 8; 6 = Fand 16; X = .X; 0 = RDB; C = CAT 

	FandXFile* XF = nullptr;
	FandTFile* TF = nullptr;

	unsigned short FirstRecPos = 0;    // for Fand 16 it's = [6] (first 6 Bytes are file header)
	unsigned char Drive = 0;           // 1=A, 2=B, else 0
	FileUseMode UMode = Closed;
	LockMode LMode = NullMode;
	LockMode ExLMode = NullMode;
	LockMode TaLMode = NullMode;

	size_t ReadRec(size_t rec_nr, Record* record, bool ignore_T_fields = false);
	size_t ReadRec(size_t rec_nr, uint8_t* buffer);
	size_t WriteRec(size_t rec_nr, Record* record);
	void CreateRec(int n, Record* record);
	void DeleteRec(int n, Record* record);
	size_t PutRec(Record* record, int& i_rec);
	size_t PutRec(uint8_t* record);

	void CompileRecLen();
	int UsedFileSize() const;
	bool IsShared();
	void Reset();

	void IncNRecs(int n);
	void DecNRecs(int n);

	uint16_t RdPrefix();
	int RdPrefixes();
	void WrPrefix();
	void WrPrefixes();

	void TruncFile();
	LockMode RewriteFile(bool append);

	void ClearUpdateFlag() override;
	void SaveFile();
	void CloseFile();
	void Close();

	//bool DeletedFlag(Record* record);
	//void ClearDeletedFlag(Record* record);
	//void SetDeletedFlag(Record* record);

	void ClearXFUpdLock();
	int XFNotValid();
	int CreateIndexFile();
	int TestXFExist();

	FileD* GetFileD();

	bool SearchKey(XString& XX, XKey* Key, int& NN, Record* record);
	int XNRecs(std::vector<XKey*>& K);
	void TryInsertAllIndexes(int RecNr, Record* record);
	void DeleteAllIndexes(int RecNr, Record* record);
	void DeleteXRec(int RecNr, Record* record);
	void OverWrXRec(int RecNr, Record* P2, Record* P, Record* record);
	void RecallRec(int recNr, Record* record);

	void GenerateNew000File(XScan* x);
	void CreateWIndex(XScan* Scan, XWKey* K, OperationType oper_type);
	void ScanSubstWIndex(XScan* Scan, std::vector<KeyFldD*>& SK, OperationType oper_type);
	void SortAndSubst(std::vector<KeyFldD*>& SK);
	void CopyIndex(XWKey* K, XKey* FromK);

	void SubstDuplF(FileD* TempFD, bool DelTF);
	void CopyDuplF(FileD* TempFD, bool DelTF);
	void IndexFileProc(bool Compress);

	//static int CopyT(FandTFile* destT00File, FandTFile* srcT00File, int srcT00Pos);
	static void CopyTFStringToH(FileD* file_d, HANDLE h, FandTFile* TF02, FileD* TFD02, int& TF02Pos);
	std::string SetTempCExt(char typ, bool isNet) const;

	std::string loadTfromPos(FieldDescr* field, int32_t pos); // due to lazy-loading of T fields in records

private:
	FileD* _parent;
	//uint8_t* _buffer; // record buffer
	bool is_null_value(FieldDescr* field_d, uint8_t* record);

	void DelTFld(FieldDescr* field_d, uint8_t* record);
	void DelAllTFlds(int32_t rec_nr);
	[[nodiscard]] std::map<FieldDescr*, int32_t> DelChangedTFields(uint8_t* orig_raw_data, Record* new_record);
	//void DelDifTFld(FieldDescr* field_d, uint8_t* record, uint8_t* comp_record);
	//void DelAllDifTFlds(uint8_t* record, uint8_t* comp_record);
	bool has_T_fields();

	bool loadB(FieldDescr* field_d, uint8_t* record);
	double loadR(FieldDescr* field_d, uint8_t* record);
	std::string loadS(FieldDescr* field_d, uint8_t* record);
	int loadT(FieldDescr* F, uint8_t* record);

	void saveB(FieldDescr* field_d, bool b, uint8_t* record);
	void saveR(FieldDescr* field_d, double r, uint8_t* record);
	void saveS(FileD* parent, FieldDescr* field_d, std::string s, uint8_t* record);
	int saveT(FieldDescr* field_d, int pos, uint8_t* record);

	std::string _extToT(const std::string& input_path);
	std::string _extToX(const std::string& dir, const std::string& name, std::string ext);

	void TestDelErr(std::string& P);

	void _getValuesFromRawData(uint8_t* buffer, Record* record, bool ignore_T_fields);
	std::unique_ptr<uint8_t[]> _getRowDataFromRecord(Record* record, const std::map<FieldDescr*, int32_t>& unchanged_T_fields, bool ignore_T_fields = false);

	// Callback function pointers
	ProgressCallbacks _msgs;
};

