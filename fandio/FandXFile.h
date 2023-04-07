#pragma once
//#include "../cppfand/constants.h"
#include "XWFile.h"

class FandXFile : public XWFile // r357
{
public:
	FandXFile() {}
	FandXFile(const FandXFile& orig);
	longint NRecs = 0, NRecsAbs = 0; // {FreeRoot..NrKeys read / written by 1 instr.}
	bool NotValid = false;
	BYTE NrKeys = 0;
	bool NoCreate = false, FirstDupl = false;
	void SetEmpty();
	void RdPrefix();
	void WrPrefix();
	void SetNotValid();
};

void XFNotValid();
void TestXFExist();
void CreateIndexFile(); // r482
void ClearXFUpdLock();
