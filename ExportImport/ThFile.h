#pragma once
#include <string>
#include <windows.h>

#include "TcFile.h"
#include "../Common/FileD.h"
#include "../Common/LongStr.h"

enum class InOutMode { _inp, _outp, _append };

class ThFile : public TcFile
{
public:
	ThFile(std::string APath, WORD CatIRec, InOutMode AMode, bool aCompress, ThFile* F);
	~ThFile() override;

	HANDLE Handle = INVALID_HANDLE_VALUE;
	std::string Path;
	std::string Vol;
	
	bool Floppy = false, IsEOL = false, Continued = false;
	//uint8_t compress = 0;
	int Size = 0, OrigSize = 0, SpaceOnDisk = 0;
	FileD* FD = nullptr;

	//bool eof = false;
	void ReadBuf();
	void WriteBuf(bool cond);
	void ClearBuf();

	//void Append();
	void Delete();
	char ForwChar();
	char RdChar();
	std::string RdDM(char Delim, short Max);
	std::string RdDelim(char Delim);
	std::string RdFix(short N);
	std::string RdLongStr();
	//void ReadBuf2();
	//void Reset();
	void ExtToT();
	//void ResetT();
	//void ResetX();
	void Rewrite();
	void RewriteT();
	void RewriteX();
	//void TestError();
	bool TestErr152();
	void WrChar(char C);
	//void WriteBuf2();
	void WrString(std::string S);
	void WrString(std::string S, bool WithDelim);
	void WrLongStr(LongStr* S, bool WithDelim);

private:
	InOutMode mode = InOutMode::_inp;
};

