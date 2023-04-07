#pragma once
//#include "../cppfand/constants.h"
#include "XPage.h"
#include "XXPage.h"

class XWFile
{
public:
	WORD UpdLockCnt = 0;
	FILE* Handle = nullptr;
	longint FreeRoot = 0;
	longint MaxPage = 0;
	void Err(WORD N);
	void TestErr();
	longint UsedFileSize();
	bool NotCached();
	void RdPage(XPage* P, longint pageNr);
	void WrPage(XPage* P, longint pageNr, bool serialize = true);
	void WrPage(XXPage* p, longint pageNr);
	longint NewPage(XPage* P);
	void ReleasePage(XPage* P, longint N);
};
