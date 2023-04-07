#pragma once
#include "XPage.h"
#include "XXPage.h"

class XWFile
{
public:
	unsigned short UpdLockCnt = 0;
	FILE* Handle = nullptr;
	int FreeRoot = 0;
	int MaxPage = 0;
	void Err(unsigned short N);
	void TestErr();
	int UsedFileSize();
	bool NotCached();
	void RdPage(XPage* P, int pageNr);
	void WrPage(XPage* P, int pageNr, bool serialize = true);
	void WrPage(XXPage* p, int pageNr);
	int NewPage(XPage* P);
	void ReleasePage(XPage* P, int N);
};
