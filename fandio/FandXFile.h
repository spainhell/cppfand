#pragma once

#include <cstdint>
#include "DataFileBase.h"
#include "XPage.h"
#include "XXPage.h"

class Fand0File;

class FandXFile : public DataFileBase
{
public:
	FandXFile(Fand0File* parent);
	FandXFile(const FandXFile& orig) = delete;
	FandXFile(const FandXFile& orig, Fand0File* parent);
	~FandXFile() override;

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
	bool Cached() const;
	bool NotCached() const;
	void CloseFile();

	void RdPage(XPage* P, int pageNr);
	void WrPage(XPage* P, int pageNr, bool serialize = true);
	void WrPage(XXPage* p, int pageNr);
	int NewPage(XPage* P);
	void ReleasePage(XPage* P, int N);

	void Err(unsigned short N);

private:
	Fand0File* _parent;
};

