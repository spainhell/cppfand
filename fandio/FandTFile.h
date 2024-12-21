#pragma once
#include <string>

#include "FandFile.h"
#include "../Common/FileEnums.h"
#include "../Common/LongStr.h"

typedef void* HANDLE;
class Fand0File;
typedef char PwCodeArr[20];

const unsigned short MPageSize = 512;
const unsigned char XPageShft = 10;
const unsigned char MPageShft = 9;

class FandTFile : public FandFile
{
public:
	FandTFile(Fand0File* parent);
	FandTFile(const FandTFile& orig) = delete;
	FandTFile(const FandTFile& orig, Fand0File* parent);
	~FandTFile() override;

	int FreePart = 0;
	bool Reserved = false, CompileProc = false, CompileAll = false;
	unsigned short IRec = 0;
	int32_t FreeRoot = 0;
	int32_t MaxPage = 0;
	double TimeStmp = 0.0;
	int LicenseNr = 0;
	int MLen = 0;
	std::string PwCode;
	std::string Pw2Code;
	enum eFormat { T00Format, DbtFormat, FptFormat } Format = T00Format;
	unsigned short FptFormatBlockSize = 0;
	bool IsWork = false;
	void Err(unsigned short n, bool ex) const;
	void TestErr() const;
	int UsedFileSize() const;
	bool NotCached() const;
	bool Cached() const;
	void RdPrefix(bool check);
	void WrPrefix();
	void SetEmpty();
	void Create(const std::string& path);
	int NewPage(bool NegL);
	void ReleasePage(int PosPg);
	void Delete(int Pos);
	LongStr* ReadLongStr(int Pos);
	std::string Read(int pos);
	int Store(char* s, size_t l);
	int Store(const std::string& s);
	void CloseFile();

private:
	Fand0File* _parent;
	void Read(size_t position, size_t count, char* buffer);
	void Write(size_t position, size_t count, char* buffer);
	void GetMLen();
	long eofPos = 0;
};
