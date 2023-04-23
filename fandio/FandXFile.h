#pragma once
#include "XWFile.h"

class FandXFile : public XWFile
{
public:
	FandXFile() {}
	FandXFile(const FandXFile& orig);
	int NRecs = 0;     // valid records count
	int NRecsAbs = 0;  // all record count (include deleted from .000 file)
	bool NotValid = false;
	unsigned char NrKeys = 0;
	bool NoCreate = false;
	bool FirstDupl = false;
	void SetEmpty();
	void RdPrefix();
	void WrPrefix();
	void SetNotValid();
};

void XFNotValid();
void TestXFExist();
void CreateIndexFile();
void ClearXFUpdLock();
