#pragma once
#include "XWFile.h"

class FandXFile : public XWFile
{
public:
	FandXFile() {}
	FandXFile(const FandXFile& orig);
	int NRecs = 0, NRecsAbs = 0; // {FreeRoot..NrKeys read / written by 1 instr.}
	bool NotValid = false;
	unsigned char NrKeys = 0;
	bool NoCreate = false, FirstDupl = false;
	void SetEmpty();
	void RdPrefix();
	void WrPrefix();
	void SetNotValid();
};

void XFNotValid();
void TestXFExist();
void CreateIndexFile();
void ClearXFUpdLock();
