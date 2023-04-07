#pragma once
#include <cstdio>
#include "FandTFile.h"
#include "FandXFile.h"

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

	WORD FrstDispl = 0;
	BYTE Drive = 0;           // 1=A, 2=B, else 0
	FileUseMode UMode = Closed;
	LockMode LMode = NullMode, ExLMode = NullMode, TaLMode = NullMode;

	longint UsedFileSize();
	bool IsShared();
	bool NotCached();
	bool Cached();
	void Reset();
};

