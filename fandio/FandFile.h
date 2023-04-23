#pragma once
#include <cstdio>
#include "FandTFile.h"
#include "FandXFile.h"
#include "../cppfand/switches.h"
#include "../cppfand/base.h"
#include "../cppfand/FieldDescr.h"
#include "../Logging/Logging.h"

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
	FandFile(const FandFile& orig);

	unsigned short RecLen = 0;
	void* RecPtr = nullptr;
	int NRecs = 0;
	bool WasWrRec = false;
	bool WasRdOnly = false;
	bool Eof = false;
	FILE* Handle = nullptr;
	FileType file_type = FileType::UNKNOWN;       // 8 = Fand 8; 6 = Fand 16; X = .X; 0 = RDB; C = CAT 

	FandXFile* XF = nullptr;
	FandTFile* TF = nullptr;

	unsigned short FrstDispl = 0;
	unsigned char Drive = 0;           // 1=A, 2=B, else 0
	FileUseMode UMode = Closed;
	LockMode LMode = NullMode, ExLMode = NullMode, TaLMode = NullMode;

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

	unsigned short RdPrefix();
	int RdPrefixes();
	void WrPrefix();
	void WrPrefixes();

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

	FileD* GetFileD();

private:
	FileD* _parent;
	double _RforD(FieldDescr* field_d, void* record);
	bool IsNullValue(void* record, WORD l);
};

