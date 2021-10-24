#pragma once
#include "../cppfand/constants.h"
#include "XPage.h"

class XWFile // r345
{
public:
	//XWFile();
	WORD UpdLockCnt = 0;
	FILE* Handle = nullptr;
	longint FreeRoot = 0, MaxPage = 0;
	void Err(WORD N);
	void TestErr();
	longint UsedFileSize();
	bool NotCached();
	void RdPage(XPage* P, longint N);
	void WrPage(XPage* P, longint N, bool serialize = true);
	longint NewPage(XPage* P);
	void ReleasePage(XPage* P, longint N);
};