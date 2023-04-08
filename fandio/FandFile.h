#pragma once
#include <cstdio>
#include "FandTFile.h"
#include "FandXFile.h"
#include "../cppfand/switches.h"
#include "../cppfand/base.h"
#include "../cppfand/FieldDescr.h"
#include "../Logging/Logging.h"

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
	FandFile();
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
	void DecNRecs(int N);
	void PutRec(void* record, int& i_rec);

	size_t RecordSize();

	// v CRecPtr vycte pozici zaznamu v .T00 souboru (ukazatel na zacatek textu)
	int _T(FieldDescr* F, void* record);
};

