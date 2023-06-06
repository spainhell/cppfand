#pragma once
#include <cstdio>
#include "FandTFile.h"
#include "FandXFile.h"
#include "../cppfand/switches.h"
#include "../cppfand/FieldDescr.h"
#include "../Logging/Logging.h"

class XWKey;
class XScan;
class FileD;

enum class FileType
{
	UNKNOWN,
	FAND8,
	FAND16,
	INDEX,
	RDB,
	CAT,
	DBF
};

class FandFile
{
public:
	FandFile(FileD* parent);
	FandFile(const FandFile& orig) = delete;
	FandFile(const FandFile& orig, FileD* parent);
	~FandFile();

	unsigned short RecLen = 0;
	void* RecPtr = nullptr;
	int NRecs = 0;
	bool WasWrRec = false;
	bool WasRdOnly = false;
	bool Eof = false;
	HANDLE Handle = nullptr;
	FileType file_type = FileType::UNKNOWN;       // 8 = Fand 8; 6 = Fand 16; X = .X; 0 = RDB; C = CAT 

	FandXFile* XF = nullptr;
	FandTFile* TF = nullptr;

	unsigned short FirstRecPos = 0;    // for Fand 16 it's 6th BYTE (first 5 Bytes is file header)
	unsigned char Drive = 0;           // 1=A, 2=B, else 0
	FileUseMode UMode = Closed;
	LockMode LMode = NullMode;
	LockMode ExLMode = NullMode;
	LockMode TaLMode = NullMode;

	int UsedFileSize();
	bool IsShared();
	bool NotCached();
	bool Cached();
	void Reset();

	void IncNRecs(int n);
	void DecNRecs(int n);
	void PutRec(void* record, int& i_rec);

	size_t RecordSize();

	bool loadB(FieldDescr* field_d, void* record);
	double loadR(FieldDescr* field_d, void* record);
	std::string loadS(FileD* parent, FieldDescr* field_d, void* record);
	pstring loadOldS(FieldDescr* field_d, void* record);
	LongStr* loadLongS(FieldDescr* field_d, void* record);
	int loadT(FieldDescr* F, void* record);

	void saveB(FieldDescr* field_d, bool b, void* record);
	void saveR(FieldDescr* field_d, double r, void* record);
	void saveS(FileD* parent, FieldDescr* field_d, std::string s, void* record);
	void saveLongS(FileD* parent, FieldDescr* field_d, LongStr* ls, void* record);
	int saveT(FieldDescr* field_d, int pos, void* record);

	void DelTFld(FieldDescr* field_d, void* record);
	void DelDifTFld(FieldDescr* field_d, void* record, void* comp_record);

	unsigned short RdPrefix();
	int RdPrefixes();
	void WrPrefix();
	void WrPrefixes();

	void TruncFile();
	LockMode RewriteFile(bool append);
	void SaveFile();
	void CloseFile();

	void SetTWorkFlag(void* record);
	bool HasTWorkFlag(void* record);
	void SetUpdFlag(void* record);
	void ClearUpdFlag(void* record);
	bool HasUpdFlag(void* record);
	bool DeletedFlag(void* record);
	void ClearDeletedFlag(void* record);
	void SetDeletedFlag(void* record);

	void ClearXFUpdLock();
	int XFNotValid();
	int CreateIndexFile();
	int TestXFExist();

	FileD* GetFileD();

	bool SearchKey(XString& XX, XKey* Key, int& NN, void* record);
	int XNRecs(std::vector<XKey*>& K);
	void TryInsertAllIndexes(int RecNr, void* record);
	void DeleteAllIndexes(int RecNr, void* record);
	void DeleteXRec(int RecNr, bool DelT, void* record);
	void OverWrXRec(int RecNr, void* P2, void* P, void* record);

	void GenerateNew000File(XScan* x, void* record);
	void CreateWIndex(XScan* Scan, XWKey* K, char Typ);
	void ScanSubstWIndex(XScan* Scan, KeyFldD* SK, char Typ);
	void SortAndSubst(KeyFldD* SK);
	void CopyIndex(XWKey* K, XKey* FromK);

	void SubstDuplF(FileD* TempFD, bool DelTF);
	void CopyDuplF(FileD* TempFD, bool DelTF);
	void IndexFileProc(bool Compress);

private:
	FileD* _parent;
	double _RforD(FieldDescr* field_d, void* record);
	bool is_null_value(void* record, WORD l);

	std::string _extToT(const std::string& input_path);
	std::string _extToX(const std::string& dir, const std::string& name, std::string ext);
};

