#pragma once
#include "FandTFile.h"
#include "FandXFile.h"
#include "../Core/switches.h"
#include "FieldDescr.h"
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
	Fand0File(FileD* parent);
	Fand0File(const Fand0File& orig) = delete;
	Fand0File(const Fand0File& orig, FileD* parent);
	~Fand0File() override;

	unsigned short RecLen = 0;
	Record* RecPtr = nullptr;
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

	size_t ReadRec(size_t rec_nr, Record* record);
	size_t WriteRec(size_t rec_nr, Record* record);
	void CreateRec(int n, Record* record);
	void DeleteRec(int n, Record* record);

	void CompileRecLen();
	int UsedFileSize() const;
	bool IsShared();
	void Reset();

	void IncNRecs(int n);
	void DecNRecs(int n);
	void PutRec(void* record, int& i_rec);

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
	void DelAllDifTFlds(uint8_t* record, uint8_t* comp_record);

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

	//void SetTWorkFlag(uint8_t* record) const;
	//bool HasTWorkFlag(uint8_t* record) const;

	void SetRecordUpdateFlag(uint8_t* record);
	void ClearRecordUpdateFlag(uint8_t* record);
	bool HasRecordUpdateFlag(const uint8_t* record);

	bool DeletedFlag(uint8_t* record);
	void ClearDeletedFlag(uint8_t* record);
	void SetDeletedFlag(uint8_t* record);

	void ClearXFUpdLock();
	int XFNotValid();
	int CreateIndexFile();
	int TestXFExist();

	FileD* GetFileD();

	bool SearchKey(XString& XX, XKey* Key, int& NN, Record* record);
	int XNRecs(std::vector<XKey*>& K);
	void TryInsertAllIndexes(int RecNr, Record* record);
	void DeleteAllIndexes(int RecNr, Record* record);
	void DeleteXRec(int RecNr, bool DelT, Record* record);
	void OverWrXRec(int RecNr, Record* P2, Record* P, Record* record);
	void RecallRec(int recNr, Record* record);

	void GenerateNew000File(XScan* x, Record* record, void (*msgFuncUpdate)(int32_t));
	void CreateWIndex(XScan* Scan, XWKey* K, OperationType oper_type);
	void ScanSubstWIndex(XScan* Scan, std::vector<KeyFldD*>& SK, OperationType oper_type);
	void SortAndSubst(std::vector<KeyFldD*>& SK, void (*msgFuncOn)(int8_t, int32_t), void (*msgFuncUpdate)(int32_t), void (*msgFuncOff)());
	void CopyIndex(XWKey* K, XKey* FromK);

	void SubstDuplF(FileD* TempFD, bool DelTF);
	void CopyDuplF(FileD* TempFD, bool DelTF);
	void IndexFileProc(bool Compress);

	//static int CopyT(FandTFile* destT00File, FandTFile* srcT00File, int srcT00Pos);
	static void CopyTFStringToH(FileD* file_d, HANDLE h, FandTFile* TF02, FileD* TFD02, int& TF02Pos);
	std::string SetTempCExt(char typ, bool isNet) const;

private:
	FileD* _parent;
	bool is_null_value(FieldDescr* field_d, uint8_t* record);

	std::string _extToT(const std::string& input_path);
	std::string _extToX(const std::string& dir, const std::string& name, std::string ext);

	void TestDelErr(std::string& P);
};

