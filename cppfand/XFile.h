#pragma once
#include "../cppfand/constants.h"
#include "../Indexes/XWFile.h"

class XFile : public XWFile // r357
{
public:
	XFile() {}
	XFile(const XFile& orig);
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
