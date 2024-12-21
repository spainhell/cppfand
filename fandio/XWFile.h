#pragma once
#include "FandFile.h"
#include "XPage.h"
#include "XXPage.h"


class Fand0File;

class XWFile : public FandFile
{
public:
	XWFile(Fand0File* parent);
	unsigned short UpdLockCnt = 0;
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

	void SetUpdateFlag();
	void ClearUpdateFlag();
	bool HasUpdateFlag();

protected:
	Fand0File* _parent;
	bool _updateFlag;
};
