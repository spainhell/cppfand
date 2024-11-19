#pragma once
#include "XPage.h"
#include "XXPage.h"

typedef void* HANDLE;
class FandFile;

class XWFile
{
public:
	XWFile(FandFile* parent);
	unsigned short UpdLockCnt = 0;
	HANDLE Handle = nullptr;
	int FreeRoot = 0;
	int MaxPage = 0;
	void Err(unsigned short N);
	void TestErr();
	int UsedFileSize();
	bool NotCached();
	bool Cached();
	void RdPage(XPage* P, int pageNr);
	void WrPage(XPage* P, int pageNr, bool serialize = true);
	void WrPage(XXPage* p, int pageNr);
	int NewPage(XPage* P);
	void ReleasePage(XPage* P, int N);

protected:
	FandFile* _parent;
};
