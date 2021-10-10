#pragma once
#include <string>

#include "TcFile.h"
#include "../cppfand/FileD.h"

enum InOutMode { _inp, _outp, _append };

const size_t BUFFER_SIZE = 16384;

class ThFile
{
public:
	ThFile(std::string APath, WORD CatIRec, InOutMode AMode, BYTE aCompress, ThFile* F);
	~ThFile();

	FILE* Handle = nullptr;
	std::string Path;
	std::string Vol;
	InOutMode Mode = _inp;
	bool Floppy = false, IsEOL = false, Continued = false;
	BYTE compress = 0;
	longint Size = 0, OrigSize = 0, SpaceOnDisk = 0;
	FileD* FD = nullptr;

	size_t lBuf = BUFFER_SIZE;
	BYTE Buf[BUFFER_SIZE]{ '\0' };
	
	bool eof = false;
	void ReadBuf();
	void WriteBuf(bool cond);
	void ClearBuf();

	//void Append();
	//void Delete();
	//char ForwChar();
	//char RdChar();
	//std::string RdDM(char Delim, integer Max);
	//std::string RdDelim(char Delim);
	//std::string RdFix(integer N);
	//LongStr* RdLongStr();
	//void ReadBuf2();
	//void Reset();
	//void ExtToT();
	//void ResetT();
	//void ResetX();
	//void Rewrite();
	//void RewriteT();
	//void RewriteX();
	//void TestError();
	//bool TestErr152();
	//void WrChar(char C);
	//void WriteBuf2();
	//void WrString(std::string S);
	//void WrLongStr(LongStr* S, bool WithDelim);
};
