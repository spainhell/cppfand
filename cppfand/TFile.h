#pragma once
#include "base.h"
#include "constants.h"

typedef char PwCodeArr[20];

const WORD MPageSize = 512;
const BYTE XPageShft = 10;
const BYTE MPageShft = 9;

class TFile
{
public:
	TFile() {}
	TFile(const TFile& orig);
	FILE* Handle = nullptr;
	longint FreePart = 0;
	bool Reserved = false, CompileProc = false, CompileAll = false;
	WORD IRec = 0;
	longint FreeRoot = 0, MaxPage = 0;
	double TimeStmp = 0.0;
	integer LicenseNr = 0;
	longint MLen = 0;
	std::string PwCode;
	std::string Pw2Code;
	enum eFormat { T00Format, DbtFormat, FptFormat } Format = T00Format;
	WORD BlockSize = 0; // FptFormat
	bool IsWork = false;
	void Err(WORD n, bool ex);
	void TestErr();
	longint UsedFileSize();
	bool NotCached();
	bool Cached();
	void RdPrefix(bool Chk);
	void WrPrefix();
	void SetEmpty();
	void Create();
	longint NewPage(bool NegL);
	void ReleasePage(int PosPg);
	void Delete(int Pos);
	LongStr* Read(int Pos);
	longint Store(char* s, size_t l);

private:
	void RdWr(FileOperation operation, size_t position, size_t count, char* buffer);
	void GetMLen();
	long eofPos = 0;
};
typedef TFile* TFilePtr;

WORD RdPrefix();
void RdPrefixes();
void WrPrefix();
void WrPrefixes();