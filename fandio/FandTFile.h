#pragma once
#include <cstdio>
#include <string>
#include "../Common/FileEnums.h"
#include "../Common/LongStr.h"

typedef char PwCodeArr[20];

const unsigned short MPageSize = 512;
const unsigned char XPageShft = 10;
const unsigned char MPageShft = 9;

class FandTFile
{
public:
	FandTFile() {}
	FandTFile(const FandTFile& orig);
	FILE* Handle = nullptr;
	int FreePart = 0;
	bool Reserved = false, CompileProc = false, CompileAll = false;
	unsigned short IRec = 0;
	__int32 FreeRoot = 0, MaxPage = 0;
	double TimeStmp = 0.0;
	int LicenseNr = 0;
	int MLen = 0;
	std::string PwCode;
	std::string Pw2Code;
	enum eFormat { T00Format, DbtFormat, FptFormat } Format = T00Format;
	unsigned short FptFormatBlockSize = 0;
	bool IsWork = false;
	void Err(unsigned short n, bool ex);
	void TestErr();
	int UsedFileSize();
	bool NotCached();
	bool Cached();
	void RdPrefix(bool Chk);
	void WrPrefix();
	void SetEmpty();
	void Create();
	int NewPage(bool NegL);
	void ReleasePage(int PosPg);
	void Delete(int Pos);
	LongStr* Read(int Pos);
	int Store(char* s, size_t l);

private:
	void RdWr(FileOperation operation, size_t position, size_t count, char* buffer);
	void GetMLen();
	long eofPos = 0;
};
typedef FandTFile* TFilePtr;

unsigned short RdPrefix();
void RdPrefixes();
void WrPrefix();
void WrPrefixes();