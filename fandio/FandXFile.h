#pragma once
#include "XWFile.h"

class FandXFile : public XWFile
{
public:
	FandXFile(Fand0File* parent);
	FandXFile(const FandXFile& orig) = delete;
	FandXFile(const FandXFile& orig, Fand0File* parent);
	~FandXFile() override;

	int NRecs = 0;     // valid records count
	int NRecsAbs = 0;  // all record count (include deleted from .000 file)
	bool NotValid = false;
	unsigned char NrKeys = 0;
	bool NoCreate = false;
	bool FirstDupl = false;
	void SetEmpty(int recs, unsigned char keys);
	void RdPrefix();
	void WrPrefix(int recs, unsigned char keys);
	void SetNotValid(int recs, unsigned char keys);
	void ClearUpdLock();
	int XFNotValid(int recs, unsigned char keys);
	void CloseFile();
};

