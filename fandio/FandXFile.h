#pragma once
#include "XWFile.h"

class FandXFile : public FandFile
{
public:
	FandXFile(Fand0File* parent);
	FandXFile(const FandXFile& orig) = delete;
	FandXFile(const FandXFile& orig, Fand0File* parent);
	~FandXFile(); //override;

	int32_t NRecs = 0;     // valid records count
	int32_t NRecsAbs = 0;  // all record count (include deleted from .000 file)
	bool NotValid = false;
	unsigned char NrKeys = 0;
	bool NoCreate = false;
	bool FirstDupl = false;
	int32_t FreeRoot = 0;
	int32_t MaxPage = 0;
	uint16_t UpdLockCnt = 0;

	void SetEmpty(int recs, unsigned char keys);
	void RdPrefix();
	void WrPrefix(int recs, unsigned char keys);
	void SetNotValid(int recs, unsigned char keys);
	void TestErr();
	int UsedFileSize();
	void ClearUpdLock();
	int32_t XFNotValid(int recs, unsigned char keys);
	void CloseFile();

private:
	Fand0File* _parent;

	void WrPage(XPage* P, int pageNr, bool serialize = true);
	void Err(unsigned short N);
};

